#    Organic Photovoltaic Device Model - a drift diffusion base/Shockley-Read-Hall
#    model for organic solar cells. 
#    Copyright (C) 2012 Roderick C. I. MacKenzie
#
#	roderick.mackenzie@nottingham.ac.uk
#	www.opvdm.com
#	Room B86 Coates, University Park, Nottingham, NG7 2RD, UK
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License v2.0, as published by
#    the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

import pygtk
pygtk.require('2.0')
import gtk
import sys
import os
import shutil
from numpy import *
from matplotlib.figure import Figure
from numpy import arange, sin, pi
from matplotlib.backends.backend_gtkagg import FigureCanvasGTKAgg as FigureCanvas
from matplotlib.backends.backend_gtkagg import NavigationToolbar2GTKAgg as NavigationToolbar
import gobject
from scan_item import scan_item_add
from inp import inp_load_file
from inp import inp_read_next_item
from gui_util import dlg_get_text
from inp import inp_get_token_value
import matplotlib.mlab as mlab
from inp import inp_write_lines_to_file
import webbrowser
from util import fx_with_units
from inp import inp_search_token_value
from cal_path import get_image_file_path
from scan_item import scan_remove_file
from scan_item import scan_item_add
from debug import debug_mode
import i18n
_ = i18n.language.gettext

(
SEG_LENGTH,
SEG_DFX,
SEG_MUL
) = range(3)

mesh_articles = []

class tab_fxmesh(gtk.VBox):
	lines=[]
	edit_list=[]

	line_number=[]
	save_file_name=""

	file_name=""
	name=""
	visible=1

	def save_data(self):
		file_name="fxmesh"+str(self.index)+".inp"
		scan_remove_file(file_name)

		out_text=[]
		out_text.append("#fx_start")
		out_text.append(str(float(self.fx_start)))
		out_text.append("#fx_segments")
		out_text.append(str(int(len(self.store))))
		i=0
		for line in self.store:
			out_text.append("#fx_segment"+str(i)+"_len")
			scan_item_add(file_name,out_text[len(out_text)-1],_("Part ")+str(i)+_(" period"),1)
			out_text.append(str(line[SEG_LENGTH]))

			out_text.append("#fx_segment"+str(i)+"_dfx")
			scan_item_add(file_name,out_text[len(out_text)-1],_("Part ")+str(i)+_(" dfx"),1)
			out_text.append(str(line[SEG_DFX]))

			out_text.append("#fx_segment"+str(i)+"_mul")
			scan_item_add(file_name,out_text[len(out_text)-1],_("Part ")+str(i)+_(" mul"),1)
			out_text.append(str(line[SEG_MUL]))
			i=i+1

		out_text.append("#ver")
		out_text.append("1.0")
		out_text.append("#end")
		
		inp_write_lines_to_file(os.path.join(os.getcwd(),file_name),out_text)
		self.update_scan_tokens()

	def update_scan_tokens(self):
		file_name="fxmesh"+str(self.index)+".inp"
		scan_remove_file(file_name)

		for i in range(0,len(self.list)):
			scan_item_add(file_name,"#fx_segment"+str(i)+"_len",_("Part ")+str(i)+_(" period"),1)
			scan_item_add(file_name,"#fx_segment"+str(i)+"_dfx",_("Part ")+str(i)+_(" dfx"),1)
			scan_item_add(file_name,"#fx_segment"+str(i)+"_mul",_("Part ")+str(i)+_(" mul"),1)


	def callback_add_section(self, widget, treeview):
		data=["0.0", "0.0", "1.0"]
		selection = treeview.get_selection()
		model, iter = selection.get_selected()

		if iter:
			path = model.get_path(iter)[0]
			self.store.insert(path+1,data)
		else:
			self.store.append(data)

		self.build_mesh()
		self.draw_graph()
		self.fig.canvas.draw()
		self.save_data()

	def callback_remove_item(self, button, treeview):

		selection = treeview.get_selection()
		model, iter = selection.get_selected()

		if iter:
			path = model.get_path(iter)[0]
			model.remove(iter)

		self.build_mesh()

		self.draw_graph()
		self.fig.canvas.draw()
		self.save_data()

	def callback_move_down(self, widget, treeview):

		selection = treeview.get_selection()
		model, iter = selection.get_selected()

		if iter:
			path = model.get_path(iter)[0]
 			self.store.move_after( iter,self.store.iter_next(iter))

		self.build_mesh()
		self.draw_graph()
		self.fig.canvas.draw()
		self.save_data()

	def callback_start_fx(self, widget, treeview):
		new_fx=dlg_get_text( _("Enter the start frequency of the simulation"), str(self.fx_start))

		if new_fx!=None:
			self.fx_start=float(new_fx)
			self.build_mesh()
			self.draw_graph()
			self.fig.canvas.draw()
			self.save_data()

	def update(self):
		self.build_mesh()
		self.draw_graph()
		self.fig.canvas.draw()

	def on_cell_edited_length(self, cell, path, new_text, model):
		model[path][SEG_LENGTH] = new_text
		self.build_mesh()
		self.draw_graph()
		self.fig.canvas.draw()
		self.save_data()

	def on_cell_edited_dfx(self, cell, path, new_text, model):
		#print "Rod",path
		model[path][SEG_DFX] = new_text
		self.build_mesh()
		self.draw_graph()
		self.fig.canvas.draw()
		self.save_data()

	def on_cell_edited_mul(self, cell, path, new_text, model):
		model[path][SEG_MUL] = new_text
		self.build_mesh()
		self.draw_graph()
		self.fig.canvas.draw()
		self.save_data()

	def gaussian(self,x, mu, sig):
		return exp(-power(x - mu, 2.) / (2 * power(sig, 2.)))

	def draw_graph(self):

		n=0
		if (len(self.fx)>0):
			mul,unit=fx_with_units(float(self.fx[len(self.fx)-1]-self.fx[0]))
		else:
			mul=1.0
			unit="Hz"

		fx=[]
		for i in range(0,len(self.fx)):
			fx.append(self.fx[i]*mul)

		self.fig.clf()
		self.fig.subplots_adjust(bottom=0.2)
		self.fig.subplots_adjust(left=0.1)
		self.ax1 = self.fig.add_subplot(111)
		self.ax1.ticklabel_format(useOffset=False)
		#ax2 = ax1.twinx()
		x_pos=0.0
		layer=0
		color =['r','g','b','y','o','r','g','b','y','o']

		self.ax1.set_ylabel(_("Magnitude (Volts)"))

		frequency, = self.ax1.plot(fx,self.mag, 'ro-', linewidth=3 ,alpha=1.0)
		self.ax1.set_xlabel(_("Frequency (")+unit+')')

		#self.fig.legend((frequency), (_("Frequency")), 'upper right')
	


	def save_image(self,file_name):
		self.fig.savefig(file_name)

	def callback_save(self, widget, data=None):
		dialog = gtk.FileChooserDialog(_("Save as.."),
                               None,
                               gtk.FILE_CHOOSER_ACTION_SAVE,
                               (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                gtk.STOCK_SAVE, gtk.RESPONSE_OK))
		dialog.set_default_response(gtk.RESPONSE_OK)

		filter = gtk.FileFilter()
		filter.set_name(".jpg")
		filter.add_pattern("*.jpg")
		dialog.add_filter(filter)

		response = dialog.run()
		if response == gtk.RESPONSE_OK:
			file_name=dialog.get_filename()

			if os.path.splitext(file_name)[1]:
				self.save_image(file_name)
			else:
				filter=dialog.get_filter()
				self.save_image(file_name+filter.get_name())
			
		elif response == gtk.RESPONSE_CANCEL:
		    print _("Closed, no files selected")
		dialog.destroy()

	def callback_help(self, widget, data=None):
		webbrowser.open('http://www.opvdm.com/man/index.html')

	def create_model(self):
		store = gtk.ListStore(str, str, str)

		for line in self.list:
			store.append([str(line[SEG_LENGTH]), str(line[SEG_DFX]), str(line[SEG_MUL])])

		return store

	def create_columns(self, treeview):

		model=treeview.get_model()
		renderer = gtk.CellRendererText()
		renderer.connect("edited", self.on_cell_edited_length, model)
		renderer.set_property('editable', True)
		column = gtk.TreeViewColumn(_("Frequency"), renderer, text=SEG_LENGTH)
		column.set_sort_column_id(SEG_LENGTH)
		treeview.append_column(column)

		renderer = gtk.CellRendererText()
		renderer.connect("edited", self.on_cell_edited_dfx, model)
		column = gtk.TreeViewColumn("dfx", renderer, text=SEG_DFX)
		renderer.set_property('editable', True)
		column.set_sort_column_id(SEG_DFX)
		treeview.append_column(column)

		renderer = gtk.CellRendererText()
		renderer.connect("edited", self.on_cell_edited_mul, model)
		renderer.set_property('editable', True)
		column = gtk.TreeViewColumn(_("Multiply"), renderer, text=SEG_MUL)
		column.set_sort_column_id(SEG_MUL)
		treeview.append_column(column)

	def load_data(self):

		lines=[]
		self.start_fx=0.0
		self.list=[]

		file_name="fxmesh"+str(self.index)+".inp"

		ret=inp_load_file(lines,file_name)
		if ret==True:
			if inp_search_token_value(lines, "#ver")=="1.0":
				pos=0
				token,value,pos=inp_read_next_item(lines,pos)
				self.fx_start=float(value)

				token,value,pos=inp_read_next_item(lines,pos)
				segments=int(value)

				for i in range(0, segments):
					token,length,pos=inp_read_next_item(lines,pos)
					token,dfx,pos=inp_read_next_item(lines,pos)
					token,mul,pos=inp_read_next_item(lines,pos)
					self.list.append((length,dfx,mul))

				print self.list
				return True
			else:
				print "file "+file_name+"wrong version"
				exit("")
				return False
		else:
			print "file "+file_name+" not found"
			return False

		return False

	def build_mesh(self):
		self.mag=[]
		self.fx=[]
		pos=self.fx_start

		seg=0
		for line in self.store:
			end_fx=pos+float(line[SEG_LENGTH])
			dfx=float(line[SEG_DFX])
			mul=float(line[SEG_MUL])

			if dfx!=0.0 and mul!=0.0:
				while(pos<end_fx):
					self.fx.append(pos)
					self.mag.append(1.0)
					pos=pos+dfx

					dfx=dfx*mul

			seg=seg+1


		self.statusbar.push(0, str(len(self.fx))+_(" mesh points"))
		

	def init(self,index):
		self.index=index
		self.fig = Figure(figsize=(5,4), dpi=100)
		self.ax1=None
		self.show_key=True
		self.hbox=gtk.HBox()
		self.edit_list=[]
		self.line_number=[]
		gui_pos=0

		self.list=[]

		self.load_data()
		self.update_scan_tokens()

		gui_pos=gui_pos+1

		canvas = FigureCanvas(self.fig)  # a gtk.DrawingArea
		#canvas.set_background('white')
		#canvas.set_facecolor('white')
		canvas.figure.patch.set_facecolor('white')
		canvas.set_size_request(500, 150)
		canvas.show()

		tooltips = gtk.Tooltips()

		toolbar = gtk.Toolbar()
		#toolbar.set_orientation(gtk.ORIENTATION_VERTICAL)
		toolbar.set_style(gtk.TOOLBAR_ICONS)
		toolbar.set_size_request(-1, 50)

		self.store = self.create_model()
		treeview = gtk.TreeView(self.store)
		treeview.show()
		tool_bar_pos=0

		save = gtk.ToolButton(gtk.STOCK_SAVE)
		tooltips.set_tip(save, _("Save image"))
		save.connect("clicked", self.callback_save)
		toolbar.insert(save, tool_bar_pos)
		tool_bar_pos=tool_bar_pos+1

		add_section = gtk.ToolButton(gtk.STOCK_ADD)
		tooltips.set_tip(add_section, _("Add section"))
		add_section.connect("clicked", self.callback_add_section,treeview)
		toolbar.insert(add_section, tool_bar_pos)
		tool_bar_pos=tool_bar_pos+1

		add_section = gtk.ToolButton(gtk.STOCK_CLEAR)
		tooltips.set_tip(add_section, _("Delete section"))
		add_section.connect("clicked", self.callback_remove_item,treeview)
		toolbar.insert(add_section, tool_bar_pos)
		tool_bar_pos=tool_bar_pos+1

		move_down = gtk.ToolButton(gtk.STOCK_GO_DOWN)
		tooltips.set_tip(move_down, _("Move down"))
		move_down.connect("clicked", self.callback_move_down,treeview)
		toolbar.insert(move_down, tool_bar_pos)
		tool_bar_pos=tool_bar_pos+1

		image = gtk.Image()
   		image.set_from_file(os.path.join(get_image_file_path(),"start.png"))
		start = gtk.ToolButton(image)
		tooltips.set_tip(start, _("Simulation start frequency"))
		start.connect("clicked", self.callback_start_fx,treeview)
		toolbar.insert(start, tool_bar_pos)
		tool_bar_pos=tool_bar_pos+1

		plot_toolbar = NavigationToolbar(self.fig.canvas, self)
		plot_toolbar.show()
		box=gtk.HBox(True, 1)
		box.set_size_request(300,-1)
		box.show()
		box.pack_start(plot_toolbar, True, True, 0)
		tb_comboitem = gtk.ToolItem();
		tb_comboitem.add(box);
		tb_comboitem.show()
		toolbar.insert(tb_comboitem, tool_bar_pos)
		tool_bar_pos=tool_bar_pos+1

		sep = gtk.SeparatorToolItem()
		sep.set_draw(False)
		sep.set_expand(True)
		toolbar.insert(sep, tool_bar_pos)
		sep.show()
		tool_bar_pos=tool_bar_pos+1

		help = gtk.ToolButton(gtk.STOCK_HELP)
		toolbar.insert(help, tool_bar_pos)
		help.connect("clicked", self.callback_help)
		help.show()
		tool_bar_pos=tool_bar_pos+1

		toolbar.show_all()
		self.pack_start(toolbar, False, True, 0)
		self.pack_start(toolbar, True, True, 0)
		tool_bar_pos=tool_bar_pos+1



		canvas.set_size_request(700,400)
		self.pack_start(canvas, True, True, 0)


		treeview.set_rules_hint(True)

		self.create_columns(treeview)

		self.pack_start(treeview, False, False, 0)

		self.statusbar = gtk.Statusbar()
		self.statusbar.show()
		self.pack_start(self.statusbar, False, False, 0)

		self.build_mesh()
		self.draw_graph()

		self.show()

