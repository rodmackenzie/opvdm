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
#include <math.h>
#include "sim.h"
#include "i.h"
#include "buffer.h"

static double n_count = 0.0;
static double p_count = 0.0;
static double rn_count = 0.0;
static double rp_count = 0.0;

void get_avg_np_pos(struct device *in, double *nx, double *px)
{
	int i;
	double navg = 0.0;
	double pavg = 0.0;
	double nsum = 0.0;
	double psum = 0.0;

	for (i = 0; i < in->ymeshpoints; i++) {
		navg +=
		    (in->n[i] + in->nt_all[i] - in->n_orig[i]) * in->ymesh[i];
		pavg +=
		    (in->p[i] + in->pt_all[i] - in->p_orig[i]) * in->ymesh[i];
		nsum += (in->n[i] + in->nt_all[i] - in->n_orig[i]);
		psum += in->p[i] + in->pt_all[i] - in->p_orig[i];

	}
	if (nsum != 0.0) {
		*nx = navg / nsum;
//printf("%le\n",navg/nsum);
	} else {
		*nx = 0.0;
	}

	if (psum != 0.0) {
		*px = pavg / psum;
	} else {
		*px = 0.0;
	}

}

double get_charge_change(struct device *in)
{

	double diff = 0.0;
	return diff;
}

//double get_charge_change(struct device *in)
//{
//double n=0.0;
//double p=0.0;
//int i=0;
//double diff=0.0;
//if (in->go_time==TRUE)
//{
//
//      for (i=0;i<in->ymeshpoints;i++)
//      {
//              n=0.0;
//              p=0.0;
//
//              n+=in->nt_save[i];
//              p+=in->pt_save[i];
//
//
//              n+=in->nf_save[i];
//              p+=in->pf_save[i];
//
//              diff+=fabs(in->p[i]+in->pt_all[i]-in->n[i]-in->nt_all[i]+n-p);
//      }
//      //printf("%le\n",diff);
//      diff/=(n+p);
//      diff*=100.0;
//}
//return diff;
//}

double get_avg_recom(struct device *in)
{
	int i;
	double sum = 0.0;
	double add = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		add = in->ptrap_to_n[i] + in->ntrap_to_p[i];
		if ((in->interfaceleft == TRUE) && (i == 0))
			add /= 2.0;
		if ((in->interfaceright == TRUE) && (i == in->ymeshpoints - 1))
			add /= 2.0;
		sum += add;
	}

	return sum / (((double)in->ymeshpoints));
}

double get_avg_relax_n(struct device *in)
{
	int i = 0;
	double Rtot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		Rtot += (in->nrelax[i]);
	}
	return Rtot / (((double)in->ymeshpoints));
}

double get_avg_relax_p(struct device *in)
{
	int i = 0;
	double Rtot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		Rtot += (in->prelax[i]);
	}
	return Rtot / (((double)in->ymeshpoints));
}

//Note this calculates the real free electron recombination rate
//not the removal of electrons from the band
double get_avg_recom_n(struct device *in)
{
	int i = 0;
	double Rtot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		Rtot += (in->ptrap_to_n[i]);
	}
	return Rtot / (((double)in->ymeshpoints));
}

//Note this calculates the real free hole recombination rate
//not the removal of holes from the band
double get_avg_recom_p(struct device *in)
{
	int i = 0;
	double Rtot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		Rtot += (in->ntrap_to_p[i]);
	}
	return Rtot / (((double)in->ymeshpoints));
}

double get_avg_Rn(struct device *in)
{
	int i = 0;
	double Rtot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		Rtot += (in->Rn[i]);
	}
	return Rtot / (((double)in->ymeshpoints));
}

double get_avg_Rp(struct device *in)
{
	int i = 0;
	double Rtot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		Rtot += (in->Rp[i]);
	}
	return Rtot / (((double)in->ymeshpoints));
}

double get_avg_k(struct device *in)
{
	double n = (get_extracted_n(in) * get_extracted_p(in));
	double R = (get_avg_recom_n(in) + get_avg_recom_p(in)) / 2.0;
	double k = R / n;
	return k;
}

double carrier_count_get_n(struct device *in)
{
	return n_count;
}

double carrier_count_get_p(struct device *in)
{
	return p_count;
}

double carrier_count_get_rn(struct device *in)
{
	return rn_count;
}

double carrier_count_get_rp(struct device *in)
{
	return rp_count;
}

void carrier_count_reset(struct device *in)
{
	n_count = 0.0;
	p_count = 0.0;
	rn_count = 0.0;
	rp_count = 0.0;
}

double get_J_recom(struct device *in)
{
	int i = 0;
	double Rtot = 0.0;
	double add = 0.0;
	double dx = in->ymesh[1] - in->ymesh[0];
	for (i = 0; i < in->ymeshpoints; i++) {
		add =
		    (in->Rn[i] - in->Gn[i] - in->Rp[i] - in->Gn[i]) * dx / 2.0;
		if ((in->interfaceleft == TRUE) && (i == 0))
			add /= 2.0;
		if ((in->interfaceright == TRUE) && (i == in->ymeshpoints - 1))
			add /= 2.0;
		Rtot += add;
	}

	return Rtot * Q;
}

double get_J_recom_n(struct device *in)
{
	int i = 0;
	double Rtot = 0.0;
	double dx = in->ymesh[1] - in->ymesh[0];
	for (i = 0; i < in->ymeshpoints; i++) {
		Rtot += (in->Rn[i]) * dx;
	}
	return Rtot;
}

double get_J_recom_p(struct device *in)
{
	int i = 0;
	double Rtot = 0.0;
	double dx = in->ymesh[1] - in->ymesh[0];
	for (i = 0; i < in->ymeshpoints; i++) {
		Rtot += (in->Rp[i]) * dx;
	}
	return Rtot;
}

double get_avg_J(struct device *in)
{
	int i = 0;
	double Jtot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		Jtot += in->Jn[i] + in->Jp[i];
	}
	return Jtot / (double)in->ymeshpoints;
}

double get_jn_avg(struct device *in)
{
	int i = 0;
	double Jtot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		Jtot += in->Jn[i];
	}
	return Jtot / (double)in->ymeshpoints;
}

double get_jp_avg(struct device *in)
{
	int i = 0;
	double Jtot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		Jtot += in->Jp[i];
	}
	return Jtot / (double)in->ymeshpoints;
}

void carrier_count_add(struct device *in)
{
	int i;
	double locat_n_tot = 0.0;
	double locat_p_tot = 0.0;

	n_count += (in->Jn[0] + in->Jn[in->ymeshpoints - 1]) * in->dt / Q;
	p_count += (in->Jp[0] + in->Jp[in->ymeshpoints - 1]) * in->dt / Q;

	double dx = in->ymesh[1] - in->ymesh[0];
//printf("\n\n%e %e\n\n",dx*(double)in->ymeshpoints,in->ymesh[in->ymeshpoints-1]);
	for (i = 0; i < in->ymeshpoints; i++) {
		locat_n_tot += in->Rfree[i] * dx;
		locat_p_tot += in->Rfree[i] * dx;
	}
//printf("%e\n",in->ymesh[in->ymeshpoints-1]);
//getchar();
	rn_count += locat_n_tot * in->dt;
	rp_count += locat_p_tot * in->dt;

//printf("\n%e %e\n",rn_count,rp_count);
}

double get_extracted_np(struct device *in)
{
	return (get_extracted_n(in) + get_extracted_p(in)) / 2.0;
}

double get_extracted_n(struct device *in)
{
	int i;
	double sum_n = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		sum_n += (in->n[i] - in->nf_save[i]);
		sum_n += (in->nt_all[i] - in->nt_save[i]);
//printf("%le %le %le\n",in->n[i]-in->nfinit[i],in->nt_all[i],in->ntinit[i]);
//getchar();
	}

	return sum_n / ((double)(in->ymeshpoints));
}

double get_total_np(struct device *in)
{
	int i;
	double sum = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		sum += (in->n[i] + in->p[i]);
		sum += (in->nt_all[i] + in->pt_all[i]);
	}

	return sum / ((double)(in->ymeshpoints) * 2.0);
}

double get_extracted_p(struct device *in)
{
	int i;

	double sum_p = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		sum_p += (in->p[i] - in->pf_save[i]);
		sum_p += (in->pt_all[i] - in->pt_save[i]);
	}

	return sum_p / ((double)(in->ymeshpoints));
}

double get_background_charge(struct device *in)
{
	int i;
	double sum = 0.0;

	for (i = 0; i < in->ymeshpoints; i++) {
		sum +=
		    (in->nf_save[i] + in->nt_save[i] + in->pf_save[i] +
		     in->pt_save[i]) / 2.0;
	}
	return sum / ((double)(in->ymeshpoints));
}

double get_extracted_k(struct device *in)
{
	int i;

	double tot = 0.0;
	double n = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += in->Rfree[i];
		n += in->n[i] * in->p[i];
	}
//printf("%le %le %le\n",tot,n,tot/n);
//getchar();
	return tot / n;
}

double get_avg_gen(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->Gn[i] + in->Gp[i]) / 2.0;
	}
	return tot / (((double)in->ymeshpoints));
}

double get_avg_mue(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += in->mun[i] * in->n[i] / (in->nt_all[i] + in->n[i]);
	}
	return tot / (((double)in->ymeshpoints));
}

double get_avg_muh(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += in->mup[i] * in->p[i] / (in->pt_all[i] + in->p[i]);
	}
	return tot / (((double)in->ymeshpoints));
}

double get_np_tot(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->n[i] + in->p[i] + in->pt_all[i] + in->nt_all[i]);
	}

	return tot / (((double)in->ymeshpoints)) / 2.0;
}

double get_free_np_avg(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->n[i] + in->p[i] - in->nf_save[i] - in->pf_save[i]);
	}

	return tot / (((double)in->ymeshpoints)) / 2.0;
}

double get_free_n_charge(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->n[i]);
	}

	return tot / (((double)in->ymeshpoints));
}

double get_free_p_charge(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->p[i]);
	}

	return tot / (((double)in->ymeshpoints));
}

double get_charge_tot(struct device *in)
{
	int i;
	double tot = 0.0;

	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->p[i] - in->n[i] + in->pt_all[i] - in->nt_all[i]);
	}

	return tot;
}

void set_orig_charge_den(struct device *in)
{
	int i;
	for (i = 0; i < in->ymeshpoints; i++) {
		in->n_orig[i] = in->n[i] + in->nt_all[i];
		in->p_orig[i] = in->p[i] + in->pt_all[i];

		in->n_orig_f[i] = in->n[i];
		in->p_orig_f[i] = in->p[i];

		in->n_orig_t[i] = in->nt_all[i];
		in->p_orig_t[i] = in->pt_all[i];
	}
}

double get_free_n_charge_delta(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->n[i] - in->n_orig_f[i]);
	}

	return tot / (((double)in->ymeshpoints));
}

double get_free_p_charge_delta(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->p[i] - in->p_orig_f[i]);
	}

	return tot / (((double)in->ymeshpoints));
}

void reset_np_save(struct device *in)
{
	int i;
	int band;
	for (i = 0; i < in->ymeshpoints; i++) {
		in->nf_save[i] = in->n[i];
		in->pf_save[i] = in->p[i];
		in->nt_save[i] = in->nt_all[i];
		in->pt_save[i] = in->pt_all[i];
		in->phi_save[i] = in->phi[i];
		for (band = 0; band < in->srh_bands; band++) {
			in->ntb_save[i][band] = in->nt[i][band];
			in->ptb_save[i][band] = in->pt[i][band];
		}
	}
}

void reset_npequlib(struct device *in)
{
	int i;
	for (i = 0; i < in->ymeshpoints; i++) {
		in->nfequlib[i] = in->n[i];
		in->pfequlib[i] = in->p[i];
		in->ntequlib[i] = in->nt_all[i];
		in->ptequlib[i] = in->pt_all[i];
	}
}

double get_n_trapped_charge(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->nt_all[i]);
	}

	return tot / in->ymeshpoints;
}

double get_p_trapped_charge(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->pt_all[i]);
	}

	return tot / in->ymeshpoints;
}

double get_charge_delta(struct device *in)
{
	int i;
	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot +=
		    (((in->n[i] + in->nt_all[i]) - in->n_orig[i]) +
		     ((in->p[i] + in->pt_all[i]) - in->p_orig[i])) / 2.0;
	}

	return tot / in->ymeshpoints;
}

double get_n_trapped_charge_delta(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->nt_all[i] - in->n_orig_t[i]);
	}

	return tot / in->ymeshpoints;
}

double get_p_trapped_charge_delta(struct device *in)
{
	int i;

	double tot = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		tot += (in->pt_all[i] - in->p_orig_t[i]);
	}

	return tot / in->ymeshpoints;
}

double get_I_recomb(struct device *in)
{
	double ret = 0.0;

	ret = (get_J_recom(in)) * (in->xlen * in->zlen) / 2.0;

	return ret;
}

double get_I(struct device *in)
{
	double ret = 0.0;

	ret += (get_J_left(in) + get_J_right(in)) * (in->xlen * in->zlen) / 2.0;

	return ret;
}

double get_i_intergration(struct device *in)
{
	int i;
	double ret = 0.0;

	for (i = 0; i < in->ymeshpoints; i++) {
		ret += (in->Jn[i] + in->Jp[i]);
	}

	ret *= (in->xlen * in->zlen) / ((double)(in->ymeshpoints));
	return ret;
}

double get_equiv_I(struct device *in)
{
	double Iout = 0.0;
	Iout = get_equiv_J(in) * (in->xlen * in->zlen);
	return Iout;
}

double get_equiv_J(struct device *in)
{
	double J = 0.0;
	J = get_J(in);
	if (in->lr_pcontact == RIGHT)
		J *= -1.0;
	J += in->Vapplied / in->Rshunt / in->area;
//printf("%e %e %e\n",(in->xlen*in->zlen),in->Rshunt,in->area);
	return J;
}

double get_I_ce(struct device *in)
{
	double ret = in->Vapplied / (in->Rcontact + in->Rshort);
	return ret;
}

double get_avg_field(struct device *in)
{
	return (in->phi[in->ymeshpoints - 1] - in->phi[0]);
}

void cal_J_drift_diffusion(struct device *in)
{
	double Ecl = 0.0;
	double Ecr = 0.0;
	double dEc = 0.0;
	double nl = 0.0;
	double nr = 0.0;
	double dn = 0.0;
	double pl = 0.0;
	double pr = 0.0;
	double dp = 0.0;
	double xl = 0.0;
	double xr = 0.0;
	double dx = 0.0;

	int i = 0;

	for (i = 0; i < in->ymeshpoints; i++) {
		if (i == 0) {
			nl = in->n[0];
			pl = in->p[0];
			Ecl = in->Ec[0];
			xl = in->ymesh[0];
		} else {
			nl = in->n[i - 1];
			pl = in->p[i - 1];
			Ecl = in->Ec[i - 1];
			xl = in->ymesh[i - 1];
		}

		if (i == in->ymeshpoints - 1) {
			nr = in->n[in->ymeshpoints - 1];
			pr = in->p[in->ymeshpoints - 1];
			Ecr = in->Ec[in->ymeshpoints - 1];
			xr = in->ymesh[in->ymeshpoints - 1];
		} else {
			nr = in->n[i + 1];
			pr = in->p[i + 1];
			Ecr = in->Ec[i + 1];
			xr = in->ymesh[i + 1];
		}
		dn = (nr - nl);
		dp = (pr - pl);
		dEc = Ecr - Ecl;
		dx = (xr - xl);
		in->Jn_diffusion[i] = Q * in->Dn[i] * dn / dx;
		in->Jn_drift[i] = Q * in->mun[i] * in->n[i] * dEc / dx;

		in->Jp_diffusion[i] = -Q * in->Dp[i] * dp / dx;
		in->Jp_drift[i] = Q * in->mup[i] * in->p[i] * dEc / dx;
	}

}

double get_Jn_diffusion(struct device *in)
{
	int i = 0;
	double J = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		J += fabs(in->Jn_diffusion[i]);
	}

	return J;
}

double get_Jn_drift(struct device *in)
{
	int i = 0;
	double J = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		J += fabs(in->Jn_drift[i]);
	}

	return J;
}

double get_Jp_diffusion(struct device *in)
{
	int i = 0;
	double J = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		J += fabs(in->Jp_diffusion[i]);
	}

	return J;
}

double get_Jp_drift(struct device *in)
{
	int i = 0;
	double J = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {
		J += fabs(in->Jp_drift[i]);
	}

	return J;
}

double get_equiv_V(struct device *in)
{
	double J = 0.0;
//if (in->adv_sim==FALSE)
//{
//J=get_J_recom(in);
//}else
//{
	J = get_equiv_J(in);
//}
//printf("%e\n",in->Rcontact);
	double V = J * in->Rcontact * in->area + in->Vapplied;
	return V;
}

double get_J(struct device *in)
{
//int i;
	double ret = 0.0;

	ret = (get_J_left(in) + get_J_right(in)) / 2.0;

	return ret;
}

double get_J_left(struct device *in)
{

	double ret = 0.0;
	ret = in->Jpleft + in->Jnleft;
//printf("%le %le\n",in->Jpleft,in->Jnleft);
	return ret * Q;
}

double get_J_right(struct device *in)
{

	double ret = 0.0;
	ret = in->Jnright + in->Jpright;
//printf("%e %e\n",in->Jn[0][0],in->Jn[0][in->ymeshpoints-2]);
//getchar();
	return ret * Q;
}
