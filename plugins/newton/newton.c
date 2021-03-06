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
#include "../../sim.h"
#include "../../dump.h"
#include "../../solver.h"
#include "../../newton.h"

static double Jnl = 0.0;
static double Jnr = 0.0;
static double Jpl = 0.0;
static double Jpr = 0.0;

static double Dnl = 0.0;
static double Dnc = 0.0;
static double Dnr = 0.0;
static double Dpl = 0.0;
static double Dpc = 0.0;
static double Dpr = 0.0;

static double nl = 0.0;
static double nc = 0.0;
static double nr = 0.0;

static double pl = 0.0;
static double pc = 0.0;
static double pr = 0.0;

static double xil = 0.0;
static double xir = 0.0;
static double xipl = 0.0;
static double xipr = 0.0;

static double dJpdxipl = 0.0;
static double dJpdxipc = 0.0;
static double dJpdxipr = 0.0;

static double dnl = 0.0;
static double dnc = 0.0;
static double dnr = 0.0;

static double dpl = 0.0;
static double dpc = 0.0;
static double dpr = 0.0;

static double munl = 0.0;
static double munc = 0.0;
static double munr = 0.0;

static double mupl = 0.0;
static double mupc = 0.0;
static double mupr = 0.0;

static double wnl = 0.0;
static double wnc = 0.0;
static double wnr = 0.0;

static double wpl = 0.0;
static double wpc = 0.0;
static double wpr = 0.0;

static double dJdxil = 0.0;
static double dJdxic = 0.0;
static double dJdxir = 0.0;

static double dJdphil = 0.0;
static double dJdphic = 0.0;
static double dJdphir = 0.0;

static double dJpdphil = 0.0;
static double dJpdphic = 0.0;
static double dJpdphir = 0.0;

static double dphidxic = 0.0;
static double dphidxipc = 0.0;

static double *dntrap = NULL;
static double *dntrapdntrap = NULL;
static double *dntrapdn = NULL;
static double *dntrapdp = NULL;
static double *dJdtrapn = NULL;
static double *dJpdtrapn = NULL;

static double *dptrapdp = NULL;
static double *dptrapdptrap = NULL;
static double *dptrap = NULL;
static double *dptrapdn = NULL;
static double *dJpdtrapp = NULL;
static double *dJdtrapp = NULL;
static double *dphidntrap = NULL;
static double *dphidptrap = NULL;
static int newton_min_ittr;

void newton_set_min_ittr(int ittr)
{
	newton_min_ittr = ittr;
}

void update_solver_vars(struct device *in, int clamp)
{

	int i;
	int band = 0;

	double clamp_temp = 300.0;

	double update = 0.0;
	for (i = 0; i < in->ymeshpoints; i++) {

		update = in->b[i];
		if ((in->interfaceleft == TRUE) && (i == 0)) {
		} else
		    if ((in->interfaceright == TRUE)
			&& (i == in->ymeshpoints - 1)) {
		} else {
			if (clamp == TRUE) {
				in->phi[i] +=
				    update / (1.0 +
					      fabs(update /
						   in->electrical_clamp /
						   (clamp_temp * kb / Q)));
			} else {
				in->phi[i] += update;

			}
		}

		update = (in->b[in->ymeshpoints * (1) + i]);
		if (clamp == TRUE) {
			in->x[i] +=
			    update / (1.0 +
				      fabs(update / in->electrical_clamp /
					   (clamp_temp * kb / Q)));
		} else {
			in->x[i] += update;
		}

		update = (in->b[in->ymeshpoints * (1 + 1) + i]);
		if (clamp == TRUE) {
			in->xp[i] += update / (1.0 + fabs(update / in->electrical_clamp / (clamp_temp * kb / Q)));	//
		} else {
			in->xp[i] += update;

		}

		if (in->ntrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				update =
				    (in->
				     b[in->ymeshpoints * (1 + 1 + 1 + band) +
				       i]);
				if (clamp == TRUE) {
					in->xt[i][band] +=
					    update / (1.0 +
						      fabs(update /
							   in->
							   electrical_clamp /
							   (clamp_temp * kb /
							    Q)));

				} else {
					in->xt[i][band] += update;
				}
			}
		}

		if (in->ptrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				update =
				    (in->
				     b[in->ymeshpoints *
				       (1 + 1 + 1 + in->srh_bands + band) + i]);
				if (clamp == TRUE) {
					in->xpt[i][band] +=
					    update / (1.0 +
						      fabs(update /
							   in->
							   electrical_clamp /
							   (clamp_temp * kb /
							    Q)));
				} else {
					in->xpt[i][band] += update;

				}
			}
		}

	}

	update_arrays(in);

}

void fill_matrix(struct device *in)
{

//double offset=-0.5;
	int band = 0;

	update_arrays(in);

//FILE *file_j =fopen("myj.dat","w");
//getchar();
	double phil;
	double phic;
	double phir;
	double yl;
	double yc;
	double yr;
	double dyl;
	double dyr;
	double ddh = 0.0;
//double dh;
	int pos = 0;

	double Ecl = 0.0;
	double Ecr = 0.0;
	double Ecc = 0.0;
	double Evl = 0.0;
	double Evr = 0.0;
	double Evc = 0.0;

	double Tel = 0.0;
//double Tec=0.0;
	double Ter = 0.0;

	double Thl = 0.0;
//double Thc=0.0;
	double Thr = 0.0;

	double xnr;
	double tnr;
	double xnl;
	double tnl;

	double xpr;
	double tpr;
	double xpl;
	double tpl;

//double exl;
//double exr;
//double exc;
//double Dexl;
//double Dexc;
//double Dexr;
//double R;

	double epr;
	double epc;
	double epl;

//double G;
	double Gn;
	double Gp;
	int i;
	double dJdxipc = 0.0;
	double dJpdxic = 0.0;

	double e0 = 0.0;
	double e1 = 0.0;

	double dphil = 0.0;
	double dphic = 0.0;
	double dphir = 0.0;
	double deriv = 0.0;

//double kll=0.0;
//double klc=0.0;
//double klr=0.0;

//double kl0=0.0;
//double kl1=0.0;

	double one = 0.0;
	double one0_l = 0.0;
	double one0_r = 0.0;

	double Rtrapn = 0.0;
	double Rtrapp = 0.0;

	double dJdphil_leftl = 0.0;
	double dJdphil_leftc = 0.0;
	double dJpdphil_leftl = 0.0;
	double dJpdphil_leftc = 0.0;
	double dphil_left = 0.0;
	double dJdxil_leftc = 0.0;
	double dJpdxipl_leftc = 0.0;

	double dJdxic_rightc = 0.0;
	double dJpdxipc_rightc = 0.0;
	double dJpdphi_rightc = 0.0;
	double dJdphi_rightc = 0.0;

	double Bfree = 0.0;
	double nceq = 0.0;
	double pceq = 0.0;
	double Rfree = 0.0;

	double nc0_l = 0.0;
//double dnc0_l=0.0;
//double pc0_l=0.0;
//double dpc0_l=0.0;

	double nc0_r = 0.0;
//double dnc0_r=0.0;
//double pc0_r=0.0;
//double dpc0_r=0.0;

	double dJnldxil_l = 0.0;
	double dJnldxil_c = 0.0;
	double dJnrdxir_c = 0.0;
	double dJnrdxir_r = 0.0;
	double dJpldxipl_l = 0.0;
	double dJpldxipl_c = 0.0;
	double dJprdxipr_c = 0.0;
	double dJprdxipr_r = 0.0;

	double i0 = 0.0;
	double didv = 0.0;	//not a function
	double diphic = 0.0;	//could be a function
	double didxic = 0.0;
	double didxipc = 0.0;
	double didphir = 0.0;
	double didxir = 0.0;
	double didxipr = 0.0;

//double dylh_left=0.0;
//double dyrh_left=0.0;
//double dncdphic=0.0;
//double dpcdphic=0.0;

	if (in->kl_in_newton == FALSE) {
		if (in->interfaceleft == TRUE) {
			in->phi[0] = in->Vapplied;
		}
	}

	if (in->interfaceright == TRUE) {
		in->phi[in->ymeshpoints - 1] = in->Vr;
	}

	pos = 0;
	for (i = 0; i < in->ymeshpoints; i++) {
		if (i == 0) {
//                              exl=0.0;
//                              Dexl=in->Dex[0];

			phil = in->Vapplied;

			yl = in->ymesh[0] - (in->ymesh[1] - in->ymesh[0]);
//                              Tll=in->Tll;
			Tel = in->Tll;
			Thl = in->Tll;

			Ecl = -in->Xi[0] - phil;
			Evl = -in->Xi[0] - phil - in->Eg[0];
			epl = in->epsilonr[0] * epsilon0;

			xnl = in->Fi[0];
			tnl = in->Xi[0];
			one = xnl + tnl;

			nl = get_n_den(one, Tel, in->imat[i]);
			dnl = get_dn_den(one, Tel, in->imat[i]);
			wnl = get_n_w(one, Tel, in->imat[i]);

			munl = in->mun[0];

			xpl = -in->Fi[0];
			tpl = in->Xi[0] + in->Eg[0];
			one = xpl - tpl;

			pl = get_p_den(one, Thl, in->imat[i]);
			dpl = get_dp_den(one, Thl, in->imat[i]);
			wpl = get_p_w(one, Thl, in->imat[i]);

			mupl = in->mup[0];

			//printf("left n= %e p= %e \n",nl,pl);

//                              kll=in->kl[i];

		} else {
//                              Dexl=in->Dex[i-1];
//                              exl=in->ex[i-1];                                
			phil = in->phi[i - 1];
			yl = in->ymesh[i - 1];
//                              Tll=in->Tl[i-1];
			Tel = in->Te[i - 1];
			Thl = in->Th[i - 1];
			Ecl = in->Ec[i - 1];
			Evl = in->Ev[i - 1];

			nl = in->n[i - 1];
			dnl = in->dn[i - 1];

			wnl = in->wn[i - 1];
			wpl = in->wp[i - 1];

			pl = in->p[i - 1];
			dpl = in->dp[i - 1];
			munl = in->mun[i - 1];
			mupl = in->mup[i - 1];

			epl = in->epsilonr[i - 1] * epsilon0;

//                              kll=in->kl[i-1];
		}

		Ecc = -in->Xi[i] - in->phi[i];
		Evc = -in->Xi[i] - in->phi[i] - in->Eg[i];

		if (i == (in->ymeshpoints - 1)) {

//                              Dexr=in->Dex[i];
//                              exr=0.0;                        
			//phir=in->Vr;

			phir = in->Vr;

			yr = in->ymesh[i] + (in->ymesh[i] - in->ymesh[i - 1]);
//                              Tlr=in->Tlr;
			Ter = in->Tlr;
			Thr = in->Tlr;

			Ecr = -in->Xi[i] - phir;
			Evr = -in->Xi[i] - phir - in->Eg[i];

			xnr = in->Vr + in->Fi[i];
			tnr = in->Xi[i];

			one = xnr + tnr;

			nr = get_n_den(one, Ter, in->imat[i]);
			dnr = get_dn_den(one, Ter, in->imat[i]);
			wnr = get_n_w(one, Ter, in->imat[i]);

			xpr = -(in->Vr + in->Fi[i]);
			tpr = in->Xi[i] + in->Eg[i];

			one = xpr - tpr;

			pr = get_p_den(one, Thr, in->imat[i]);
			dpr = get_dp_den(one, Thr, in->imat[i]);
			wpr = get_p_w(one, Thr, in->imat[i]);

			munr = in->mun[i];
			mupr = in->mup[i];

			//printf("right n= %e p= %e \n",nr,pr);

			epr = in->epsilonr[i] * epsilon0;
//                              klr=in->kl[i];

		} else {

//                              Dexr=in->Dex[i+1];
//                              exr=in->ex[i+1];                                
			phir = in->phi[i + 1];
			yr = in->ymesh[i + 1];
//                              Tlr=in->Tl[i+1];
			Ter = in->Te[i + 1];
			Thr = in->Th[i + 1];

			Ecr = in->Ec[i + 1];
			Evr = in->Ev[i + 1];

			nr = in->n[i + 1];
			dnr = in->dn[i + 1];

			wnr = in->wn[i + 1];
			wpr = in->wp[i + 1];

			pr = in->p[i + 1];
			dpr = in->dp[i + 1];
			munr = in->mun[i + 1];
			mupr = in->mup[i + 1];

			epr = in->epsilonr[i + 1] * epsilon0;
//                              klr=in->kl[i+1];

		}

		dJdxipc = 0.0;
		dJpdxic = 0.0;

		epc = in->epsilonr[i] * epsilon0;

//                      exc=in->ex[i];
//                      Dexc=in->Dex[i];
		yc = in->ymesh[i];
		dyl = yc - yl;
		dyr = yr - yc;
		ddh = (dyl + dyr) / 2.0;
		double dylh = dyl / 2.0;
		double dyrh = dyr / 2.0;

//                      dh=(dyl+dyr);
		phic = in->phi[i];
//                      Tlc=in->Tl[i];
//                      Tec=in->Te[i];
//                      Thc=in->Th[i];

		munc = in->mun[i];
		mupc = in->mup[i];

		wnc = in->wn[i];
		wpc = in->wp[i];

		Dnl = munl * (2.0 / 3.0) * wnl / Q;
		Dpl = mupl * (2.0 / 3.0) * wpl / Q;

		Dnc = munc * (2.0 / 3.0) * wnc / Q;
		Dpc = mupc * (2.0 / 3.0) * wpc / Q;
		in->Dn[i] = Dnc;
		in->Dp[i] = Dnc;

		Dnr = munr * (2.0 / 3.0) * wnr / Q;
		Dpr = mupr * (2.0 / 3.0) * wpr / Q;

		Dnl = (Dnl + Dnc) / 2.0;
		Dnr = (Dnr + Dnc) / 2.0;

		Dpl = (Dpl + Dpc) / 2.0;
		Dpr = (Dpr + Dpc) / 2.0;

		munl = (munl + munc) / 2.0;
		munr = (munr + munc) / 2.0;

		mupl = (mupl + mupc) / 2.0;
		mupr = (mupr + mupc) / 2.0;

		nc = in->n[i];
		pc = in->p[i];

		dnc = in->dn[i];
		dpc = in->dp[i];
//                              dncdphic=in->dndphi[i];
//                              dpcdphic=in->dpdphi[i];

		Bfree = in->B[i];
		nceq = in->nfequlib[i];
		pceq = in->pfequlib[i];
		Rfree = Bfree * (nc * pc - nceq * pceq);
		in->Rfree[i] = Rfree;
		//if (in->ymeshpoints*(1)+i==31) printf("Rod -- %d %le %le %le \n",in->ymeshpoints*(1)+i,Rfree,nceq,pceq);

//                      klc=in->kl[i];

//      R=in->R[i];
		Gn = in->Gn[i];
		Gp = in->Gp[i];

		e0 = (epl + epc) / 2.0;
		e1 = (epc + epr) / 2.0;

//      kl0=(klc+kll)/2.0;
//      kl1=(klr+klc)/2.0;

		dphil = -e0 / dyl / ddh;
		dphic = e0 / dyl / ddh + e1 / dyr / ddh;
		dphir = -e1 / dyr / ddh;

		double dphil_d = dphil;
		double dphic_d = dphic;
		double dphir_d = dphir;

		//if (i==in->ymeshpoints-1)
		//{
		//printf("%le\n",phir);
//
		//}
		deriv = phil * dphil + phic * dphic + phir * dphir;

		dphidxic = Q * (dnc);
		dphidxipc = -Q * (dpc);

		if (((in->interfaceleft == TRUE) && (i == 0))
		    || ((in->interfaceright == TRUE)
			&& (i == in->ymeshpoints - 1))) {
			dphidxic = 0.0;
			dphidxipc = 0.0;
		}

		if (in->ntrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				dphidntrap[band] = Q * (in->dnt[i][band]);
				if ((in->interfaceleft == TRUE) && (i == 0))
					dphidntrap[band] = 0.0;
				if ((in->interfaceright == TRUE)
				    && (i == in->ymeshpoints - 1))
					dphidntrap[band] = 0.0;
			}
		}

		if (in->ptrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				dphidptrap[band] = -Q * (in->dpt[i][band]);
				if ((in->interfaceleft == TRUE) && (i == 0))
					dphidptrap[band] = 0.0;
				if ((in->interfaceright == TRUE)
				    && (i == in->ymeshpoints - 1))
					dphidptrap[band] = 0.0;

				//dphidxipc+=-Q*(in->dpt[i]);
			}
		}

//      G=in->G[i];

		xil = Q * 2.0 * (3.0 / 2.0) * (Ecc - Ecl) / ((wnc + wnl));
		xir = Q * 2.0 * (3.0 / 2.0) * (Ecr - Ecc) / ((wnr + wnc));

		//double dxil=-Q*2.0*(3.0/2.0)*(Ecc-Ecl)/pow((wnc+wnl),2.0);
		//double dxir=-Q*2.0*(3.0/2.0)*(Ecr-Ecc)/pow((wnr+wnc),2.0);

		xipl = Q * 2.0 * (3.0 / 2.0) * (Evc - Evl) / (wpc + wpl);
		xipr = Q * 2.0 * (3.0 / 2.0) * (Evr - Evc) / (wpr + wpc);

		dJdxil = 0.0;
		dJdxic = 0.0;
		dJdxir = 0.0;

		dJpdxipl = 0.0;
		dJpdxipc = 0.0;
		dJpdxipr = 0.0;

		dJdphil = 0.0;
		dJdphic = 0.0;
		dJdphir = 0.0;

		dJpdphil = 0.0;
		dJpdphic = 0.0;
		dJpdphir = 0.0;

		Jnl = (Dnl / dyl) * (B(-xil) * nc - B(xil) * nl);
		dJnldxil_l = -(Dnl / dyl) * (B(xil) * dnl);
		dJnldxil_c = (Dnl / dyl) * B(-xil) * dnc;

		double dJnldphi_l =
		    -(munl / dyl) * (dB(-xil) * nc + dB(xil) * nl);
		double dJnldphi_c =
		    (munl / dyl) * (dB(-xil) * nc + dB(xil) * nl);

		Jnr = (Dnr / dyr) * (B(-xir) * nr - B(xir) * nc);
		dJnrdxir_c = -(Dnr / dyr) * (B(xir) * dnc);
		dJnrdxir_r = (Dnr / dyr) * (B(-xir) * dnr);

		double dJnrdphi_c =
		    (munr / dyr) * (-dB(-xir) * nr - dB(xir) * nc);
		double dJnrdphi_r =
		    (munr / dyr) * (dB(-xir) * nr + dB(xir) * nc);

		Jpl = (Dpl / dyl) * (B(-xipl) * pl - B(xipl) * pc);
		dJpldxipl_l = (Dpl / dyl) * (B(-xipl) * dpl);
		dJpldxipl_c = -(Dpl / dyl) * B(xipl) * dpc;

		double dJpldphi_l =
		    -((mupl) / dyl) * (dB(-xipl) * pl + dB(xipl) * pc);
		double dJpldphi_c =
		    ((mupl) / dyl) * (dB(-xipl) * pl + dB(xipl) * pc);

		Jpr = (Dpr / dyr) * (B(-xipr) * pc - B(xipr) * pr);
		dJprdxipr_c = (Dpr / dyr) * (B(-xipr) * dpc);
		dJprdxipr_r = -(Dpr / dyr) * (B(xipr) * dpr);

		double dJprdphi_c =
		    -(mupr / dyr) * (dB(-xipr) * pc + dB(xipr) * pr);
		double dJprdphi_r =
		    (mupr / dyr) * (dB(-xipr) * pc + dB(xipr) * pr);

		if (i == 0) {
			//printf("%le %le %le %le\n",in->vl*(nc-nc0_l),Jnl,-in->vl*(pc-pc0_l),Jpl);
			//getchar();
			in->Jnleft = Jnl;
			in->Jpleft = Jpl;
		}

		if (i == in->ymeshpoints - 1) {
			//printf("%le %le %le %le\n",in->vr*(nc-nc0_r),Jnr,in->vr*(pc-pc0_r),Jpr);
			in->Jnright = Jnr;
			in->Jpright = Jpr;
		}
		//printf("----------\n");
		//printf("%le %le\n",Jnl,Jpl);
		//printf("%le %le\n",Jnr,Jpr);

		in->Jn[i] = Q * (Jnl + Jnr) / 2.0;
		in->Jp[i] = Q * (Jpl + Jpr) / 2.0;

		dJdxil += -dJnldxil_l / (dylh + dyrh);
		dJdxic += (-dJnldxil_c + dJnrdxir_c) / (dylh + dyrh);
		dJdxir += dJnrdxir_r / (dylh + dyrh);

		dJpdxipl += -dJpldxipl_l / (dylh + dyrh);
		dJpdxipc += (-dJpldxipl_c + dJprdxipr_c) / (dylh + dyrh);
		dJpdxipr += dJprdxipr_r / (dylh + dyrh);

		dJdphil += -dJnldphi_l / (dylh + dyrh);
		dJdphic += (-dJnldphi_c + dJnrdphi_c) / (dylh + dyrh);
		dJdphir += dJnrdphi_r / (dylh + dyrh);

		dJpdphil += -dJpldphi_l / (dylh + dyrh);
		dJpdphic += (-dJpldphi_c + dJprdphi_c) / (dylh + dyrh);
		dJpdphir += dJprdphi_r / (dylh + dyrh);

		if (Bfree != 0.0) {
			dJdxic += -Bfree * (dnc * pc);
			dJdxipc += -Bfree * (nc * dpc);

			dJpdxipc += Bfree * (nc * dpc);
			dJpdxic += Bfree * (dnc * pc);

			if ((in->interfaceleft == TRUE) && (i == 0)) {
			} else
			    if ((in->interfaceright == TRUE)
				&& (i == in->ymeshpoints - 1)) {
			} else {
				dJdphic += -Bfree * (dnc * pc);
				dJpdphic += Bfree * (nc * dpc);
			}

		}

		if (i == 0) {

			dJdphil_leftl = dJnldphi_l;
			dJdphil_leftc = dJnldphi_c;
			dJpdphil_leftl = dJpldphi_l;
			dJpdphil_leftc = dJpldphi_c;
			//printf("%le %le\n",dpc,dnc);
			dphil_left = -e0 / dyl / ddh;
			dJdxil_leftc = dJnldxil_c;
			dJpdxipl_leftc = dJpldxipl_c;
			//dylh_left=dylh;
			//dyrh_left=dyrh;

		}

		if (i == in->ymeshpoints - 1) {
			dJdxic_rightc = dJnrdxir_c;
			dJpdxipc_rightc = dJprdxipr_c;
			dJdphi_rightc = -dJnrdphi_r;
			dJpdphi_rightc = dJprdphi_c;
		}

		Rtrapn = 0.0;
		Rtrapp = 0.0;

		in->nrelax[i] = 0.0;
		in->ntrap_to_p[i] = 0.0;
		in->prelax[i] = 0.0;
		in->ptrap_to_n[i] = 0.0;

		if (in->ntrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				dJdtrapn[band] = 0.0;
				dJpdtrapn[band] = 0.0;
				dntrap[band] =
				    nc * in->srh_n_r1[i][band] -
				    in->srh_n_r2[i][band] -
				    pc * in->srh_n_r3[i][band] +
				    in->srh_n_r4[i][band];
				dntrapdntrap[band] =
				    nc * in->dsrh_n_r1[i][band] -
				    in->dsrh_n_r2[i][band] -
				    pc * in->dsrh_n_r3[i][band] +
				    in->dsrh_n_r4[i][band];
				dntrapdn[band] = dnc * in->srh_n_r1[i][band];
				dntrapdp[band] = -dpc * in->srh_n_r3[i][band];
				Rtrapn +=
				    nc * in->srh_n_r1[i][band] -
				    in->srh_n_r2[i][band];
				dJdxic -= dnc * in->srh_n_r1[i][band];
				dJdtrapn[band] -=
				    nc * in->dsrh_n_r1[i][band] -
				    in->dsrh_n_r2[i][band];
				Rtrapp +=
				    -(-pc * in->srh_n_r3[i][band] +
				      in->srh_n_r4[i][band]);
				dJpdxipc += -(-dpc * in->srh_n_r3[i][band]);
				dJpdtrapn[band] =
				    -(-pc * in->dsrh_n_r3[i][band] +
				      in->dsrh_n_r4[i][band]);

				in->nrelax[i] +=
				    nc * in->srh_n_r1[i][band] -
				    in->srh_n_r2[i][band];
				in->ntrap_to_p[i] +=
				    -(-pc * in->srh_n_r3[i][band] +
				      in->srh_n_r4[i][band]);

				in->nt_r1[i][band] = nc * in->srh_n_r1[i][band];
				in->nt_r2[i][band] = in->srh_n_r2[i][band];
				in->nt_r3[i][band] = pc * in->srh_n_r3[i][band];
				in->nt_r4[i][band] = in->srh_n_r4[i][band];

			}
		}
		//band=0;
		//printf("heren %le %le %le %le\n",in->Vapplied,nc*in->srh_n_r1[i][band]-in->srh_n_r2[i][band]-pc*in->srh_n_r3[i][band]+in->srh_n_r4[i][band],nc*in->srh_n_r1[i][band]-in->srh_n_r2[i][band],-pc*in->srh_n_r3[i][band]+in->srh_n_r4[i][band]);

		if (in->ptrapnewton == TRUE) {

			for (band = 0; band < in->srh_bands; band++) {
				//dJdtrapn[band]=0.0;
				dJpdtrapp[band] = 0.0;
				dJdtrapp[band] = 0.0;
				dptrap[band] =
				    pc * in->srh_p_r1[i][band] -
				    in->srh_p_r2[i][band] -
				    nc * in->srh_p_r3[i][band] +
				    in->srh_p_r4[i][band];
				dptrapdptrap[band] =
				    pc * in->dsrh_p_r1[i][band] -
				    in->dsrh_p_r2[i][band] -
				    nc * in->dsrh_p_r3[i][band] +
				    in->dsrh_p_r4[i][band];
				dptrapdp[band] = dpc * in->srh_p_r1[i][band];
				dptrapdn[band] = -dnc * in->srh_p_r3[i][band];

				Rtrapp +=
				    pc * in->srh_p_r1[i][band] -
				    in->srh_p_r2[i][band];
				dJpdxipc += dpc * in->srh_p_r1[i][band];
				dJpdtrapp[band] +=
				    pc * in->dsrh_p_r1[i][band] -
				    in->dsrh_p_r2[i][band];

				Rtrapn +=
				    -(-nc * in->srh_p_r3[i][band] +
				      in->srh_p_r4[i][band]);
				dJdxic -= -(-dnc * in->srh_p_r3[i][band]);
				dJdtrapp[band] -=
				    -(-nc * in->dsrh_p_r3[i][band] +
				      in->dsrh_p_r4[i][band]);

				in->prelax[i] +=
				    pc * in->srh_p_r1[i][band] -
				    in->srh_p_r2[i][band];
				in->ptrap_to_n[i] +=
				    -(-nc * in->srh_p_r3[i][band] +
				      in->srh_p_r4[i][band]);

				in->pt_r1[i][band] = pc * in->srh_p_r1[i][band];
				in->pt_r2[i][band] = in->srh_p_r2[i][band];
				in->pt_r3[i][band] = nc * in->srh_p_r3[i][band];
				in->pt_r4[i][band] = in->srh_p_r4[i][band];

			}

		}
		//band=0;
		//printf("hereh %le %le %le %le\n",in->Vapplied,pc*in->srh_p_r1[i][band]-in->srh_p_r2[i][band]-nc*in->srh_p_r3[i][band]+in->srh_p_r4[i][band],pc*in->srh_p_r1[i][band]-in->srh_p_r2[i][band],-nc*in->srh_p_r3[i][band]+in->srh_p_r4[i][band]);

		in->Rn[i] = Rtrapn;
		in->Rp[i] = Rtrapp;
		//Rtrapp=1e24;
		//Rtrapn=1e24;

		if (i != 0) {
			in->Ti[pos] = i;
			in->Tj[pos] = i - 1;
			in->Tx[pos] = dphil_d;
			pos++;
			//electron

			in->Ti[pos] = in->ymeshpoints * (1) + i;
			in->Tj[pos] = in->ymeshpoints * (1) + i - 1;
			in->Tx[pos] = dJdxil;
			pos++;

			in->Ti[pos] = in->ymeshpoints * (1) + i;
			in->Tj[pos] = i - 1;
			in->Tx[pos] = dJdphil;
			pos++;

			//hole
			in->Ti[pos] = in->ymeshpoints * (1 + 1) + i;
			in->Tj[pos] = in->ymeshpoints * (1 + 1) + i - 1;
			in->Tx[pos] = dJpdxipl;
			pos++;

			in->Ti[pos] = i + in->ymeshpoints * (1 + 1);
			in->Tj[pos] = i - 1;
			in->Tx[pos] = dJpdphil;
			pos++;

		}

		if ((in->kl_in_newton == TRUE) && (in->interfaceleft == TRUE)
		    && (i == 0)) {
			//printf("%d\n",i);
			//getchar();
		} else {
			//phi
			in->Ti[pos] = i;
			in->Tj[pos] = i;
			in->Tx[pos] = dphic_d;
			pos++;
		}

		in->Ti[pos] = i;
		in->Tj[pos] = i + in->ymeshpoints * (1);
		in->Tx[pos] = dphidxic;
		pos++;

		in->Ti[pos] = i;
		in->Tj[pos] = i + in->ymeshpoints * (1 + 1);
		in->Tx[pos] = dphidxipc;
		pos++;

		//electron

		in->Ti[pos] = in->ymeshpoints * (1) + i;
		in->Tj[pos] = in->ymeshpoints * (1) + i;
		in->Tx[pos] = dJdxic;
		pos++;

		in->Ti[pos] = in->ymeshpoints * (1) + i;
		in->Tj[pos] = in->ymeshpoints * (1 + 1) + i;
		in->Tx[pos] = dJdxipc;
		pos++;

		in->Ti[pos] = in->ymeshpoints * (1) + i;
		in->Tj[pos] = i;
		in->Tx[pos] = dJdphic;
		pos++;

		//hole
		in->Ti[pos] = in->ymeshpoints * (1 + 1) + i;
		in->Tj[pos] = in->ymeshpoints * (1 + 1) + i;
		in->Tx[pos] = dJpdxipc;
		pos++;

		in->Ti[pos] = in->ymeshpoints * (1 + 1) + i;
		in->Tj[pos] = in->ymeshpoints * (1) + i;
		in->Tx[pos] = dJpdxic;
		pos++;

		in->Ti[pos] = in->ymeshpoints * (1 + 1) + i;
		in->Tj[pos] = i;
		in->Tx[pos] = dJpdphic;
		pos++;

		if (in->ntrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				in->Ti[pos] =
				    in->ymeshpoints * (1 + 1 + 1 + band) + i;
				in->Tj[pos] =
				    in->ymeshpoints * (1 + 1 + 1 + band) + i;
				in->Tx[pos] = dntrapdntrap[band];
				pos++;

				in->Ti[pos] =
				    in->ymeshpoints * (1 + 1 + 1 + band) + i;
				in->Tj[pos] = in->ymeshpoints * 1 + i;
				in->Tx[pos] = dntrapdn[band];
				pos++;

				in->Ti[pos] =
				    in->ymeshpoints * (1 + 1 + 1 + band) + i;
				in->Tj[pos] = in->ymeshpoints * (1 + 1) + i;
				in->Tx[pos] = dntrapdp[band];
				pos++;

				in->Ti[pos] = in->ymeshpoints * (1) + i;
				in->Tj[pos] =
				    in->ymeshpoints * (1 + 1 + 1 + band) + i;
				in->Tx[pos] = dJdtrapn[band];
				pos++;

				in->Ti[pos] = in->ymeshpoints * (1 + 1) + i;
				in->Tj[pos] =
				    in->ymeshpoints * (1 + 1 + 1 + band) + i;
				in->Tx[pos] = dJpdtrapn[band];
				pos++;

				in->Ti[pos] = i;
				in->Tj[pos] =
				    in->ymeshpoints * (1 + 1 + 1 + band) + i;
				in->Tx[pos] = dphidntrap[band];
				pos++;

			}

		}

		if (in->ptrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				in->Ti[pos] =
				    in->ymeshpoints * (1 + 1 + 1 +
						       in->srh_bands + band) +
				    i;
				in->Tj[pos] =
				    in->ymeshpoints * (1 + 1 + 1 +
						       in->srh_bands + band) +
				    i;
				in->Tx[pos] = dptrapdptrap[band];
				pos++;

				in->Ti[pos] =
				    in->ymeshpoints * (1 + 1 + 1 +
						       in->srh_bands + band) +
				    i;
				in->Tj[pos] = in->ymeshpoints * (1 + 1) + i;
				in->Tx[pos] = dptrapdp[band];
				pos++;

				in->Ti[pos] =
				    in->ymeshpoints * (1 + 1 + 1 +
						       in->srh_bands + band) +
				    i;
				in->Tj[pos] = in->ymeshpoints * (1) + i;
				in->Tx[pos] = dptrapdn[band];
				pos++;

				in->Ti[pos] = in->ymeshpoints * (1 + 1) + i;
				in->Tj[pos] =
				    in->ymeshpoints * (1 + 1 + 1 +
						       in->srh_bands + band) +
				    i;
				in->Tx[pos] = dJpdtrapp[band];
				pos++;

				in->Ti[pos] = in->ymeshpoints * (1) + i;
				in->Tj[pos] =
				    in->ymeshpoints * (1 + 1 + 1 +
						       in->srh_bands + band) +
				    i;
				in->Tx[pos] = dJdtrapp[band];
				pos++;

				in->Ti[pos] = i;
				in->Tj[pos] =
				    in->ymeshpoints * (1 + 1 + 1 +
						       in->srh_bands + band) +
				    i;
				in->Tx[pos] = dphidptrap[band];
				pos++;
			}

		}

		if (i != (in->ymeshpoints - 1)) {

			if ((in->kl_in_newton == TRUE)
			    && (in->interfaceleft == TRUE) && (i == 0)) {
				//printf("%d\n",i);
				//getchar();
			} else {
				//phi
				in->Ti[pos] = i;
				in->Tj[pos] = i + 1;
				in->Tx[pos] = dphir_d;
				pos++;
			}

			//electron
			in->Ti[pos] = in->ymeshpoints * (1) + i;
			in->Tj[pos] = in->ymeshpoints * (1) + i + 1;
			in->Tx[pos] = dJdxir;
			pos++;

			in->Ti[pos] = i + in->ymeshpoints * (1);
			in->Tj[pos] = i + 1;
			in->Tx[pos] = dJdphir;
			pos++;

			//hole
			in->Ti[pos] = in->ymeshpoints * (1 + 1) + i;
			in->Tj[pos] = in->ymeshpoints * (1 + 1) + i + 1;
			in->Tx[pos] = dJpdxipr;
			pos++;

			in->Ti[pos] = i + in->ymeshpoints * (1 + 1);
			in->Tj[pos] = i + 1;
			in->Tx[pos] = dJpdphir;
			pos++;

		}
		//Possion
		if ((in->interfaceleft == TRUE) && (i == 0)) {
			in->b[i] = -0.0;
		} else
		    if ((in->interfaceright == TRUE)
			&& (i == in->ymeshpoints - 1)) {
			in->b[i] = -0.0;
		} else {
			in->b[i] = -(deriv);

			in->b[i] += -(-Q * (in->p[i] - in->n[i]));

			for (band = 0; band < in->srh_bands; band++) {
				in->b[i] +=
				    -(-Q * (in->pt[i][band] - in->nt[i][band]));
			}

			in->b[i] += -(-Q * in->Nad[i]);
		}

		in->b[in->ymeshpoints * (1) + i] =
		    -((Jnr - Jnl) / (dylh + dyrh) - Rtrapn - Rfree);

		//getchar();
		in->b[in->ymeshpoints * (1) + i] -= Gn;
		//hole
		in->b[in->ymeshpoints * (1 + 1) + i] =
		    -((Jpr - Jpl) / (dylh + dyrh) + Rtrapp + Rfree);
		//printf("%le %le\n",Rtrapn,Rtrapp);

		in->b[in->ymeshpoints * (1 + 1) + i] -= -Gp;

		if (in->ntrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				in->b[in->ymeshpoints * (1 + 1 + 1 + band) +
				      i] = -(dntrap[band]);
			}
		}

		if (in->ptrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				in->b[in->ymeshpoints *
				      (1 + 1 + 1 + in->srh_bands + band) + i] =
				    -(dptrap[band]);
			}

		}

	}

	if (pos > in->N) {
		ewe("Error %d %d %d\n", pos, in->N, in->kl_in_newton);
	}
	//solver_dump_matrix(in->M,pos,in->Ti,in->Tj, in->Tx,in->b);
//fclose(file_j);
//printf("Check J file\n");
//getchar();

}

double get_cur_error(struct device *in)
{
	int i;
	double phi = 0.0;
	double n = 0.0;
	double p = 0.0;
	double x = 0.0;
	double te = 0.0;
	double th = 0.0;
	double tl = 0.0;
	double ttn = 0.0;
	double ttp = 0.0;
	double i0 = 0.0;
	int band = 0;
	for (i = 0; i < in->ymeshpoints; i++) {
		if ((in->interfaceleft == TRUE) && (i == 0)) {
		} else
		    if ((in->interfaceright == TRUE)
			&& (i == in->ymeshpoints - 1)) {
		} else {
			phi += fabs(in->b[i]);
		}

		n += fabs(in->b[in->ymeshpoints * (1) + i]);
		p += +fabs(in->b[in->ymeshpoints * (1 + 1) + i]);

		if (in->ntrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				ttn +=
				    fabs(in->
					 b[in->ymeshpoints *
					   (1 + 1 + 1 + band) + i]);
				//printf("error %le\n",fabs(in->b[in->ymeshpoints*(1+1+1+band)+i]));
			}
		}

		if (in->ptrapnewton == TRUE) {
			for (band = 0; band < in->srh_bands; band++) {
				ttp +=
				    fabs(in->
					 b[in->ymeshpoints *
					   (1 + 1 + 1 + in->srh_bands + band) +
					   i]);
			}
		}

	}
	double tot = phi + n + p + x + te + th + tl + ttn + ttp + i0;
//printf("%e %e %e %e %e %e %e %e %e\n",phi,n,p,x,te,th,tl,ttn,ttp);
	if (isnan(tot)) {
		printf("%e %e %e %e %e %e %e %e %e\n", phi, n, p, x, te, th, tl,
		       ttn, ttp);
		solver_dump_matrix(in->M, in->N, in->Ti, in->Tj, in->Tx, in->b,
				   "");
		ewe("nan detected in newton solver\n");
	}

	return tot;
}

void solver_cal_memory(struct device *in, int *ret_N, int *ret_M)
{
	int i = 0;
	int N = 0;
	int M = 0;
	N = in->ymeshpoints * 3 - 2;	//Possion main

	N += in->ymeshpoints * 3 - 2;	//Je main
	N += in->ymeshpoints * 3 - 2;	//Jh main

	N += in->ymeshpoints * 3 - 2;	//dJe/phi
	N += in->ymeshpoints * 3 - 2;	//dJh/phi

	N += in->ymeshpoints;	//dphi/dn
	N += in->ymeshpoints;	//dphi/dh

	N += in->ymeshpoints;	//dJndp
	N += in->ymeshpoints;	//dJpdn

	M = in->ymeshpoints;	//Pos

	M += in->ymeshpoints;	//Je
	M += in->ymeshpoints;	//Jh

	if (in->ntrapnewton == TRUE) {
		for (i = 0; i < in->srh_bands; i++) {
			N += in->ymeshpoints;	//dntrapdn
			N += in->ymeshpoints;	//dntrapdntrap
			N += in->ymeshpoints;	//dntrapdp
			N += in->ymeshpoints;	//dJndtrapn
			N += in->ymeshpoints;	//dJpdtrapn
			N += in->ymeshpoints;	//dphidntrap

			M += in->ymeshpoints;	//nt
		}

	}

	if (in->ptrapnewton == TRUE) {
		for (i = 0; i < in->srh_bands; i++) {
			N += in->ymeshpoints;	//dptrapdp
			N += in->ymeshpoints;	//dptrapdptrap
			N += in->ymeshpoints;	//dptrapdn
			N += in->ymeshpoints;	//dJpdtrapp
			N += in->ymeshpoints;	//dJdtrapp
			N += in->ymeshpoints;	//dphidptrap

			M += in->ymeshpoints;	//pt
		}
	}
	*ret_N = N;
	*ret_M = M;
}

void solver_realloc(struct device *in)
{
	int N = 0;
	int M = 0;

	solver_cal_memory(in, &N, &M);

	int alloc = FALSE;
	if ((in->N == 0) || (in->M == 0)) {
		in->N = N;
		in->M = M;
		alloc = TRUE;
	} else if ((N != in->N) || (M != in->M)) {
		in->N = N;
		in->M = M;
		alloc = TRUE;
	}

	if (alloc == TRUE) {

		in->Ti = realloc(in->Ti, in->N * sizeof(int));
		if (in->Ti == NULL) {
			ewe("in->Ti - memory error\n");
		}

		in->Tj = realloc(in->Tj, in->N * sizeof(int));
		if (in->Tj == NULL) {
			ewe("in->Tj - memory error\n");
		}

		in->Tx = realloc(in->Tx, in->N * sizeof(double));
		if (in->Tx == NULL) {
			ewe("in->Tx - memory error\n");
		}

		in->b = realloc(in->b, in->M * sizeof(double));
		if (in->b == NULL) {
			ewe("in->b - memory error\n");
		}

		dntrap = realloc(dntrap, in->srh_bands * sizeof(double));
		if (dntrap == NULL) {
			ewe("dntrap - memory error\n");
		}

		dntrapdntrap =
		    realloc(dntrapdntrap, in->srh_bands * sizeof(double));
		if (dntrapdntrap == NULL) {
			ewe("dntrapdntrap - memory error\n");
		}

		dntrapdn = realloc(dntrapdn, in->srh_bands * sizeof(double));
		if (dntrapdn == NULL) {
			ewe("dntrapdn - memory error\n");
		}

		dntrapdp = realloc(dntrapdp, in->srh_bands * sizeof(double));
		if (dntrapdp == NULL) {
			ewe("dntrapdp - memory error\n");
		}

		dJdtrapn = realloc(dJdtrapn, in->srh_bands * sizeof(double));
		if (dJdtrapn == NULL) {
			ewe("dJdtrapn - memory error\n");
		}

		dJpdtrapn = realloc(dJpdtrapn, in->srh_bands * sizeof(double));
		if (dJpdtrapn == NULL) {
			ewe("dJpdtrapn - memory error\n");
		}

		dphidntrap =
		    realloc(dphidntrap, in->srh_bands * sizeof(double));
		if (dphidntrap == NULL) {
			ewe("dphidntrap - memory error\n");
		}

		dptrapdp = realloc(dptrapdp, in->srh_bands * sizeof(double));
		if (dptrapdp == NULL) {
			ewe("dptrapdp - memory error\n");
		}

		dptrapdptrap =
		    realloc(dptrapdptrap, in->srh_bands * sizeof(double));
		if (dptrapdptrap == NULL) {
			ewe("dptrapdptrap - memory error\n");
		}

		dptrap = realloc(dptrap, in->srh_bands * sizeof(double));
		if (dptrap == NULL) {
			ewe("dptrap - memory error\n");
		}

		dptrapdn = realloc(dptrapdn, in->srh_bands * sizeof(double));
		if (dptrapdn == NULL) {
			ewe("dptrapdn - memory error\n");
		}

		dJpdtrapp = realloc(dJpdtrapp, in->srh_bands * sizeof(double));
		if (dJpdtrapp == NULL) {
			ewe("dJpdtrapp - memory error\n");
		}

		dJdtrapp = realloc(dJdtrapp, in->srh_bands * sizeof(double));
		if (dJdtrapp == NULL) {
			ewe("dJdtrapp - memory error\n");
		}

		dphidptrap =
		    realloc(dphidptrap, in->srh_bands * sizeof(double));
		if (dphidptrap == NULL) {
			ewe("dphidptrap - memory error\n");
		}

	}

}

void solver_free_memory(struct device *in)
{

	free(dntrap);
	dntrap = NULL;
	free(dntrapdntrap);
	dntrapdntrap = NULL;
	free(dntrapdn);
	dntrapdn = NULL;
	free(dntrapdp);
	dntrapdp = NULL;
	free(dJdtrapn);
	dJdtrapn = NULL;
	free(dJpdtrapn);
	dJpdtrapn = NULL;
	free(dphidntrap);
	dphidntrap = NULL;

	free(dptrapdp);
	dptrapdp = NULL;
	free(dptrapdptrap);
	dptrapdptrap = NULL;
	free(dptrap);
	dptrap = NULL;
	free(dptrapdn);
	dptrapdn = NULL;
	free(dJpdtrapp);
	dJpdtrapp = NULL;
	free(dJdtrapp);
	dJdtrapp = NULL;
	free(dphidptrap);
	dphidptrap = NULL;

	free(in->Ti);
	in->Ti = NULL;
	free(in->Tj);
	in->Tj = NULL;
	free(in->Tx);
	in->Tx = NULL;
	free(in->b);
	in->b = NULL;

}

int solve_cur(struct device *in)
{
	double error;
	int ittr = 0;
	if (get_dump_status(dump_print_newtonerror) == TRUE) {
		printf("Solve cur\n");
	}

#ifdef only
	only_update_thermal = FALSE;
#endif
//in->enable_back=FALSE;
	int stop = FALSE;
	int thermalrun = 0;
	double check[10];
	int cpos = 0;
	int i = 0;
//for (i=0;i<in->ymeshpoints;i++)
//{
//      printf("Rod ------- nt= %d %le\n",i,in->Gn[i]);
//}

	do {

		fill_matrix(in);

//dump_for_plot(in);
//plot_now(in,"plot");
		//solver_dump_matrix(in->M,in->N,in->Ti,in->Tj, in->Tx,in->b);
//getchar();

		if (in->stop == TRUE) {
			break;
		}

		solver(in->M, in->N, in->Ti, in->Tj, in->Tx, in->b);

		int propper = TRUE;

		update_solver_vars(in, TRUE);
		//printf("Going to clamp=%d\n",propper);
		//solver_dump_matrix(in->M,in->N,in->Ti,in->Tj, in->Tx,in->b);
		//printf("%d\n");
		//getchar();    

		error = get_cur_error(in);

		//thermalrun++;
		if (thermalrun == 40)
			thermalrun = 0;
		//update(in);
//getchar();

		if (get_dump_status(dump_print_newtonerror) == TRUE) {
			printf("%d Cur error = %e %le I=%le", ittr, error,
			       in->Vapplied, get_I(in));

			printf("\n");
		}

		in->last_error = error;
		in->last_ittr = ittr;
		ittr++;

		if (get_dump_status(dump_write_converge) == TRUE) {
			in->converge =
			    fopena(in->outputpath, "converge.dat", "a");
			fprintf(in->converge, "%e\n", error);
			fclose(in->converge);
		}

		stop = TRUE;

		if (ittr < in->max_electrical_itt) {
			if (error > in->min_cur_error) {
				stop = FALSE;
			}
		}

		if (ittr < in->newton_min_itt) {
			stop = FALSE;
		}

		if (in->newton_clever_exit == TRUE) {
			check[cpos] = error;
			cpos++;

			if (cpos > 10) {
				cpos = 0;
			}

			if (ittr >= in->newton_min_itt) {
				if ((check[0] < error) || (check[1] < error)) {
					stop = TRUE;
				}
			}
		}

	} while (stop == FALSE);

	in->newton_last_ittr = ittr;

	if (error > 1e-3) {
		printf_log
		    ("warning: The solver has not converged very well.\n");
	}
//getchar();
	if (get_dump_status(dump_newton) == TRUE) {
		dump_1d_slice(in, in->outputpath);
	}
//plot_now(in,"plot");
//getchar();
	in->odes += in->M;
//getchar();

	return 0;
}
