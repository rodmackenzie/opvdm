//    Organic Photovoltaic Device Model - a drift diffusion base/Shockley-Read-Hall
//    model for organic solar cells. 
//    Copyright (C) 2012 Roderick C. I. MacKenzie
//
//      roderick.mackenzie@nottingham.ac.uk
//      www.roderickmackenzie.eu
//      Room B86 Coates, University Park, Nottingham, NG7 2RD, UK
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; version 2 of the License
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"
#include "dump.h"
#include <math.h>
#include "log.h"

void init_dump(struct device *in)
{
	int i = 0;
	if (get_dump_status(dump_iodump) == TRUE) {
		FILE *out;
		out = fopena(in->outputpath, "init_Fi.dat", "w");
		for (i = 0; i < in->ymeshpoints; i++) {
			fprintf(out, "%e %e\n", in->ymesh[i], in->Fi[i]);
		}
		fclose(out);

		out = fopena(in->outputpath, "init_Ec.dat", "w");
		for (i = 0; i < in->ymeshpoints; i++) {
			fprintf(out, "%e %e\n", in->ymesh[i], in->Ec[i]);
		}
		fclose(out);

		out = fopena(in->outputpath, "init_Ev.dat", "w");
		for (i = 0; i < in->ymeshpoints; i++) {
			fprintf(out, "%e %e\n", in->ymesh[i], in->Ev[i]);
		}
		fclose(out);

		out = fopena(in->outputpath, "init_n.dat", "w");
		for (i = 0; i < in->ymeshpoints; i++) {
			fprintf(out, "%e %e\n", in->ymesh[i], in->n[i]);
		}
		fclose(out);

		out = fopena(in->outputpath, "init_p.dat", "w");
		for (i = 0; i < in->ymeshpoints; i++) {
			fprintf(out, "%e %e\n", in->ymesh[i], in->p[i]);
		}
		fclose(out);
	}
}

void my_guess(struct device *in)
{
	int i;

//getchar();
	double Ef = 0.0;
	double phi_ramp = 0.0;
	double Eg = in->Eg[0];
	double Xi = in->Xi[0];
	double charge_left = in->lcharge;
	double charge_right = in->rcharge;

	double top_l = 0.0;
	double top_r = 0.0;
	if (in->interfaceleft == TRUE) {
		top_l = in->phibleft - Eg;
	} else {
		if (in->lr_pcontact == LEFT) {
			top_l =
			    get_top_from_p(charge_left, in->Te[0], in->imat[0]);
		} else {
			top_l =
			    -(in->Eg[0] +
			      get_top_from_n(charge_left, in->Te[0],
					     in->imat[0]));
		}
	}

	if (in->interfaceright == TRUE) {
		top_r = -in->phibright;
	} else {
		if (in->lr_pcontact == LEFT) {
			top_r =
			    get_top_from_n(charge_right,
					   in->Te[in->ymeshpoints - 1],
					   in->imat[in->ymeshpoints - 1]);
		} else {
			top_r =
			    -(Eg +
			      get_top_from_p(charge_right,
					     in->Te[in->ymeshpoints - 1],
					     in->imat[in->ymeshpoints - 1]));
		}
	}

	if (get_dump_status(dump_iodump) == TRUE) {
		printf_log("check1= %e %e\n",
			   get_p_den(top_l, in->Te[0], in->imat[0]),
			   charge_left);
		printf_log("check2= %e %e\n",
			   get_n_den(top_r, in->Te[in->ymeshpoints - 1],
				     in->imat[in->ymeshpoints - 1]),
			   charge_right);
	}

	double delta_phi =
	    top_l + top_r + in->Eg[0] + in->Xi[0] - in->Xi[in->ymeshpoints - 1];
	double test_l = -in->Xi[0] + top_r;
	double test_r = -in->Xi[0] - in->Eg[0] - top_l;
	in->vbi = delta_phi;
	if (get_dump_status(dump_print_text) == TRUE) {
		printf_log("delta=%le\n", delta_phi);
		printf_log(">>>>top_l= %le\n", top_l + Eg);
		printf_log(">>>>top_r= %le\n", -top_r);
		printf_log("left= %le right = %le  %le %le\n", test_l, test_r,
			   test_r - test_l, delta_phi);
		printf_log("%le %le %le %le %le\n", top_l, top_r, Eg, delta_phi,
			   in->phi[0]);
	}

	Ef = -(top_l + Xi + Eg);

	double Lp =
	    get_p_den((-in->Xi[0] - in->phi[0] - Eg) - Ef, in->Th[0],
		      in->imat[0]);
	double Ln =
	    get_n_den(Ef - (-in->Xi[0] - in->phi[0]), in->Te[0], in->imat[0]);
	double Rp =
	    get_p_den((-in->Xi[in->ymeshpoints - 1] - delta_phi - Eg) - Ef,
		      in->Th[in->ymeshpoints - 1],
		      in->imat[in->ymeshpoints - 1]);
	double Rn =
	    get_n_den(Ef - (-in->Xi[in->ymeshpoints - 1] - delta_phi),
		      in->Te[in->ymeshpoints - 1],
		      in->imat[in->ymeshpoints - 1]);

	in->l_electrons = Ln;
	in->l_holes = Lp;
	in->r_electrons = Rn;
	in->r_holes = Rp;

	if (get_dump_status(dump_iodump) == TRUE) {
		printf_log("Ef=%e\n", Ef);
		printf_log("Holes on left contact = %e\n", Lp);
		printf_log("Electrons on left contact = %e\n", Ln);

		printf_log("Holes on right contact = %e\n", Rp);
		printf_log("Electrons on right contact = %e\n", Rn);

		FILE *contacts = fopena(in->outputpath, "contacts.dat", "w");
		fprintf(contacts, "%le\n", Lp);
		fprintf(contacts, "%le\n", Ln);

		fprintf(contacts, "%le\n", Rp);
		fprintf(contacts, "%le\n", Rn);
		fclose(contacts);
	}

	int band;
	for (i = 0; i < in->ymeshpoints; i++) {
		phi_ramp =
		    delta_phi * (in->ymesh[i] / in->ymesh[in->ymeshpoints - 1]);

		in->Fi[i] = Ef;

		in->Fn[i] = Ef;
		in->Fp[i] = Ef;

		in->phi[i] = phi_ramp;

		in->x[i] = in->phi[i] + in->Fn[i];
		in->xp[i] = -(in->phi[i] + in->Fp[i]);

		in->Ec[i] = -in->phi[i] - in->Xi[i];
		if (in->Ec[i] < in->Fi[i]) {
			in->phi[i] = -(in->Fi[i] + in->Xi[i]);
			in->Ec[i] = -in->phi[i] - in->Xi[i];
		}

		in->Ev[i] = -in->phi[i] - in->Xi[i] - in->Eg[i];
		if (in->Ev[i] > in->Fi[i]) {
			in->phi[i] = -(in->Fi[i] + in->Xi[i] + in->Eg[i]);
			in->Ev[i] = -in->phi[i] - in->Xi[i] - in->Eg[i];

			in->Ec[i] = -in->phi[i] - in->Xi[i];
		}

		double t = in->Fi[i] - in->Ec[i];
		double tp = in->Ev[i] - in->Fi[i];

		in->n[i] = in->Nc[i] * exp(((t) * Q) / (kb * in->Te[i]));
		in->p[i] = in->Nv[i] * exp(((tp) * Q) / (kb * in->Th[i]));

//printf("%e %e\n",t,tp);
//getchar();
		in->mun[i] = get_n_mu(in->imat[i]);
		in->mup[i] = get_p_mu(in->imat[i]);

		for (band = 0; band < in->srh_bands; band++) {
			in->Fnt[i][band] =
			    -in->phi[i] - in->Xi[i] +
			    dos_srh_get_fermi_n(in->n[i], in->p[i], band,
						in->imat[i], in->Te[i]);
			in->Fpt[i][band] =
			    -in->phi[i] - in->Xi[i] - in->Eg[i] -
			    dos_srh_get_fermi_p(in->n[i], in->p[i], band,
						in->imat[i], in->Th[i]);

			in->xt[i][band] = in->phi[i] + in->Fnt[i][band];
			in->nt[i][band] =
			    get_n_pop_srh(in->xt[i][band] + in->tt[i],
					  in->Te[i], band, in->imat[i]);
			in->dnt[i][band] =
			    get_dn_pop_srh(in->xt[i][band] + in->tt[i],
					   in->Te[i], band, in->imat[i]);

			in->xpt[i][band] = -(in->phi[i] + in->Fpt[i][band]);
			in->pt[i][band] =
			    get_p_pop_srh(in->xpt[i][band] - in->tpt[i],
					  in->Th[i], band, in->imat[i]);
			in->dpt[i][band] =
			    get_dp_pop_srh(in->xpt[i][band] - in->tpt[i],
					   in->Th[i], band, in->imat[i]);
		}

	}

	in->Vl = 0.0;
	in->Vr = delta_phi;
	in->Vbi = delta_phi;
	init_dump(in);
//getchar();
	if (in->stoppoint == 1)
		exit(0);
}

void get_initial(struct device *in)
{
	my_guess(in);
	return;
}
