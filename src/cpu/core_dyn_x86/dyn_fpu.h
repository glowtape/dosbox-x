/*
 *  Copyright (C) 2002-2013  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "dosbox.h"
#if C_FPU

#include <math.h>
#include <float.h>
#include "cross.h"
#include "mem.h"
#include "fpu.h"
#include "cpu.h"


static void FPU_FDECSTP(){
	TOP = (TOP - 1) & 7;
}

static void FPU_FINCSTP(){
	TOP = (TOP + 1) & 7;
}

static void FPU_FNSTCW(PhysPt addr){
	mem_writew(addr,fpu.cw);
}

static void FPU_FFREE(Bitu st) {
	fpu.tags[st]=TAG_Empty;
}


#if C_FPU_X86
#include "../../fpu/fpu_instructions_x86.h"
#elif defined(HAS_LONG_DOUBLE)
#include "../../fpu/fpu_instructions_longdouble.h"
#else
#include "../../fpu/fpu_instructions.h"
#endif


#define dyn_fpu_top() {				\
	gen_protectflags();				\
	gen_load_host(&TOP,DREG(EA),4); \
	gen_dop_word_imm(DOP_ADD,true,DREG(EA),decode.modrm.rm);	\
	gen_dop_word_imm(DOP_AND,true,DREG(EA),7);	\
	gen_load_host(&TOP,DREG(TMPB),4);			\
}

static void dyn_eatree() {
	Bitu group=(decode.modrm.val >> 3) & 7;
	switch (group){
	case 0x00:		/* FADD ST,STi */
		gen_call_function((void*)&FPU_FADD_EA,"%Ddr",DREG(TMPB));
		break;
	case 0x01:		/* FMUL  ST,STi */
		gen_call_function((void*)&FPU_FMUL_EA,"%Ddr",DREG(TMPB));
		break;
	case 0x02:		/* FCOM  STi */
		gen_call_function((void*)&FPU_FCOM_EA,"%Ddr",DREG(TMPB));
		break;
	case 0x03:		/* FCOMP STi */
		gen_call_function((void*)&FPU_FCOM_EA,"%Ddr",DREG(TMPB));
		gen_call_function((void*)&FPU_FPOP,"");
		break;
	case 0x04:		/* FSUB  ST,STi */
		gen_call_function((void*)&FPU_FSUB_EA,"%Ddr",DREG(TMPB));
		break;	
	case 0x05:		/* FSUBR ST,STi */
		gen_call_function((void*)&FPU_FSUBR_EA,"%Ddr",DREG(TMPB));
		break;
	case 0x06:		/* FDIV  ST,STi */
		gen_call_function((void*)&FPU_FDIV_EA,"%Ddr",DREG(TMPB));
		break;
	case 0x07:		/* FDIVR ST,STi */
		gen_call_function((void*)&FPU_FDIVR_EA,"%Ddr",DREG(TMPB));
		break;
	default:
		break;
	}
}

static void dyn_fpu_esc0(){
	dyn_get_modrm(); 
	if (decode.modrm.val >= 0xc0) { 
		dyn_fpu_top();
		Bitu group=(decode.modrm.val >> 3) & 7;
		switch (group){
		case 0x00:		//FADD ST,STi /
			gen_call_function((void*)&FPU_FADD,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x01:		// FMUL  ST,STi /
			gen_call_function((void*)&FPU_FMUL,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x02:		// FCOM  STi /
			gen_call_function((void*)&FPU_FCOM,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x03:		// FCOMP STi /
			gen_call_function((void*)&FPU_FCOM,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		case 0x04:		// FSUB  ST,STi /
			gen_call_function((void*)&FPU_FSUB,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;	
		case 0x05:		// FSUBR ST,STi /
			gen_call_function((void*)&FPU_FSUBR,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x06:		// FDIV  ST,STi /
			gen_call_function((void*)&FPU_FDIV,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x07:		// FDIVR ST,STi /
			gen_call_function((void*)&FPU_FDIVR,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		default:
			break;
		}
	} else { 
		dyn_fill_ea();
		gen_call_function((void*)&FPU_FLD_F32_EA,"%Ddr",DREG(EA)); 
		gen_load_host(&TOP,DREG(TMPB),4);
		dyn_eatree();
	}
}

static void dyn_fpu_esc1(){
	dyn_get_modrm();  
	if (decode.modrm.val >= 0xc0) { 
		Bitu group=(decode.modrm.val >> 3) & 7;
		Bitu sub=(decode.modrm.val & 7);
		switch (group){
		case 0x00: /* FLD STi */
			gen_protectflags(); 
			gen_load_host(&TOP,DREG(EA),4); 
			gen_dop_word_imm(DOP_ADD,true,DREG(EA),decode.modrm.rm); 
			gen_dop_word_imm(DOP_AND,true,DREG(EA),7); 
			gen_call_function((void*)&FPU_PREP_PUSH,""); 
			gen_load_host(&TOP,DREG(TMPB),4); 
			gen_call_function((void*)&FPU_FST,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x01: /* FXCH STi */
			dyn_fpu_top();
			gen_call_function((void*)&FPU_FXCH,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x02: /* FNOP */
			gen_call_function((void*)&FPU_FNOP,"");
			break;
		case 0x03: /* FSTP STi */
			dyn_fpu_top();
			gen_call_function((void*)&FPU_FST,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;   
		case 0x04:
			switch(sub){
			case 0x00:       /* FCHS */
				gen_call_function((void*)&FPU_FCHS,"");
				break;
			case 0x01:       /* FABS */
				gen_call_function((void*)&FPU_FABS,"");
				break;
			case 0x02:       /* UNKNOWN */
			case 0x03:       /* ILLEGAL */
				LOG(LOG_FPU,LOG_WARN)("ESC 1:Unhandled group %X subfunction %X",group,sub);
				break;
			case 0x04:       /* FTST */
				gen_call_function((void*)&FPU_FTST,"");
				break;
			case 0x05:       /* FXAM */
				gen_call_function((void*)&FPU_FXAM,"");
				break;
			case 0x06:       /* FTSTP (cyrix)*/
			case 0x07:       /* UNKNOWN */
				LOG(LOG_FPU,LOG_WARN)("ESC 1:Unhandled group %X subfunction %X",group,sub);
				break;
			}
			break;
		case 0x05:
			switch(sub){	
			case 0x00:       /* FLD1 */
				gen_call_function((void*)&FPU_FLD1,"");
				break;
			case 0x01:       /* FLDL2T */
				gen_call_function((void*)&FPU_FLDL2T,"");
				break;
			case 0x02:       /* FLDL2E */
				gen_call_function((void*)&FPU_FLDL2E,"");
				break;
			case 0x03:       /* FLDPI */
				gen_call_function((void*)&FPU_FLDPI,"");
				break;
			case 0x04:       /* FLDLG2 */
				gen_call_function((void*)&FPU_FLDLG2,"");
				break;
			case 0x05:       /* FLDLN2 */
				gen_call_function((void*)&FPU_FLDLN2,"");
				break;
			case 0x06:       /* FLDZ*/
				gen_call_function((void*)&FPU_FLDZ,"");
				break;
			case 0x07:       /* ILLEGAL */
				LOG(LOG_FPU,LOG_WARN)("ESC 1:Unhandled group %X subfunction %X",group,sub);
				break;
			}
			break;
		case 0x06:
			switch(sub){
			case 0x00:	/* F2XM1 */
				gen_call_function((void*)&FPU_F2XM1,"");
				break;
			case 0x01:	/* FYL2X */
				gen_call_function((void*)&FPU_FYL2X,"");
				break;
			case 0x02:	/* FPTAN  */
				gen_call_function((void*)&FPU_FPTAN,"");
				break;
			case 0x03:	/* FPATAN */
				gen_call_function((void*)&FPU_FPATAN,"");
				break;
			case 0x04:	/* FXTRACT */
				gen_call_function((void*)&FPU_FXTRACT,"");
				break;
			case 0x05:	/* FPREM1 */
				gen_call_function((void*)&FPU_FPREM1,"");
				break;
			case 0x06:	/* FDECSTP */
				gen_call_function((void*)&FPU_FDECSTP,"");
				break;
			case 0x07:	/* FINCSTP */
				gen_call_function((void*)&FPU_FINCSTP,"");
				break;
			default:
				LOG(LOG_FPU,LOG_WARN)("ESC 1:Unhandled group %X subfunction %X",group,sub);
				break;
			}
			break;
		case 0x07:
			switch(sub){
			case 0x00:		/* FPREM */
				gen_call_function((void*)&FPU_FPREM,"");
				break;
			case 0x01:		/* FYL2XP1 */
				gen_call_function((void*)&FPU_FYL2XP1,"");
				break;
			case 0x02:		/* FSQRT */
				gen_call_function((void*)&FPU_FSQRT,"");
				break;
			case 0x03:		/* FSINCOS */
				gen_call_function((void*)&FPU_FSINCOS,"");
				break;
			case 0x04:		/* FRNDINT */
				gen_call_function((void*)&FPU_FRNDINT,"");
				break;
			case 0x05:		/* FSCALE */
				gen_call_function((void*)&FPU_FSCALE,"");
				break;
			case 0x06:		/* FSIN */
				gen_call_function((void*)&FPU_FSIN,"");
				break;
			case 0x07:		/* FCOS */
				gen_call_function((void*)&FPU_FCOS,"");
				break;
			default:
				LOG(LOG_FPU,LOG_WARN)("ESC 1:Unhandled group %X subfunction %X",group,sub);
				break;
			}
			break;
		default:
			LOG(LOG_FPU,LOG_WARN)("ESC 1:Unhandled group %X subfunction %X",group,sub);
			break;
		}
	} else {
		Bitu group=(decode.modrm.val >> 3) & 7;
		Bitu sub=(decode.modrm.val & 7);
		dyn_fill_ea(); 
		switch(group){
		case 0x00: /* FLD float*/
			gen_protectflags(); 
			gen_call_function((void*)&FPU_PREP_PUSH,"");
			gen_load_host(&TOP,DREG(TMPB),4); 
			gen_call_function((void*)&FPU_FLD_F32,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x01: /* UNKNOWN */
			LOG(LOG_FPU,LOG_WARN)("ESC EA 1:Unhandled group %d subfunction %d",group,sub);
			break;
		case 0x02: /* FST float*/
			gen_call_function((void*)&FPU_FST_F32,"%Ddr",DREG(EA));
			break;
		case 0x03: /* FSTP float*/
			gen_call_function((void*)&FPU_FST_F32,"%Ddr",DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		case 0x04: /* FLDENV */
			gen_call_function((void*)&FPU_FLDENV,"%Ddr",DREG(EA));
			break;
		case 0x05: /* FLDCW */
			gen_call_function((void *)&FPU_FLDCW,"%Ddr",DREG(EA));
			break;
		case 0x06: /* FSTENV */
			gen_call_function((void *)&FPU_FSTENV,"%Ddr",DREG(EA));
			break;
		case 0x07:  /* FNSTCW*/
			gen_call_function((void *)&FPU_FNSTCW,"%Ddr",DREG(EA));
			break;
		default:
			LOG(LOG_FPU,LOG_WARN)("ESC EA 1:Unhandled group %d subfunction %d",group,sub);
			break;
		}
	}
}

static void dyn_fpu_esc2(){
	dyn_get_modrm();  
	if (decode.modrm.val >= 0xc0) { 
		Bitu group=(decode.modrm.val >> 3) & 7;
		Bitu sub=(decode.modrm.val & 7);
		switch(group){
		case 0x05:
			switch(sub){
			case 0x01:		/* FUCOMPP */
				gen_protectflags(); 
				gen_load_host(&TOP,DREG(EA),4); 
				gen_dop_word_imm(DOP_ADD,true,DREG(EA),1); 
				gen_dop_word_imm(DOP_AND,true,DREG(EA),7); 
				gen_load_host(&TOP,DREG(TMPB),4); 
				gen_call_function((void *)&FPU_FUCOM,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
				gen_call_function((void *)&FPU_FPOP,"");
				gen_call_function((void *)&FPU_FPOP,"");
				break;
			default:
				LOG(LOG_FPU,LOG_WARN)("ESC 2:Unhandled group %d subfunction %d",group,sub); 
				break;
			}
			break;
		default:
	   		LOG(LOG_FPU,LOG_WARN)("ESC 2:Unhandled group %d subfunction %d",group,sub);
			break;
		}
	} else {
		dyn_fill_ea(); 
		gen_call_function((void*)&FPU_FLD_I32_EA,"%Ddr",DREG(EA)); 
		gen_load_host(&TOP,DREG(TMPB),4); 
		dyn_eatree();
	}
}

static void dyn_fpu_esc3(){
	dyn_get_modrm();  
	if (decode.modrm.val >= 0xc0) { 
		Bitu group=(decode.modrm.val >> 3) & 7;
		Bitu sub=(decode.modrm.val & 7);
		switch (group) {
		case 0x04:
			switch (sub) {
			case 0x00:				//FNENI
			case 0x01:				//FNDIS
				LOG(LOG_FPU,LOG_ERROR)("8087 only fpu code used esc 3: group 4: subfuntion :%d",sub);
				break;
			case 0x02:				//FNCLEX FCLEX
				gen_call_function((void*)&FPU_FCLEX,"");
				break;
			case 0x03:				//FNINIT FINIT
				gen_call_function((void*)&FPU_FINIT,"");
				break;
			case 0x04:				//FNSETPM
			case 0x05:				//FRSTPM
//				LOG(LOG_FPU,LOG_ERROR)("80267 protected mode (un)set. Nothing done");
				break;
			default:
				E_Exit("ESC 3:ILLEGAL OPCODE group %d subfunction %d",group,sub);
			}
			break;
		default:
			LOG(LOG_FPU,LOG_WARN)("ESC 3:Unhandled group %d subfunction %d",group,sub);
			break;
		}
	} else {
		Bitu group=(decode.modrm.val >> 3) & 7;
		Bitu sub=(decode.modrm.val & 7);
		dyn_fill_ea(); 
		switch(group){
		case 0x00:	/* FILD */
			gen_call_function((void*)&FPU_PREP_PUSH,"");
			gen_protectflags(); 
			gen_load_host(&TOP,DREG(TMPB),4); 
			gen_call_function((void*)&FPU_FLD_I32,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x01:	/* FISTTP */
			LOG(LOG_FPU,LOG_WARN)("ESC 3 EA:Unhandled group %d subfunction %d",group,sub);
			break;
		case 0x02:	/* FIST */
			gen_call_function((void*)&FPU_FST_I32,"%Ddr",DREG(EA));
			break;
		case 0x03:	/* FISTP */
			gen_call_function((void*)&FPU_FST_I32,"%Ddr",DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		case 0x05:	/* FLD 80 Bits Real */
			gen_call_function((void*)&FPU_PREP_PUSH,"");
			gen_call_function((void*)&FPU_FLD_F80,"%Ddr",DREG(EA));
			break;
		case 0x07:	/* FSTP 80 Bits Real */
			gen_call_function((void*)&FPU_FST_F80,"%Ddr",DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		default:
			LOG(LOG_FPU,LOG_WARN)("ESC 3 EA:Unhandled group %d subfunction %d",group,sub);
		}
	}
}

static void dyn_fpu_esc4(){
	dyn_get_modrm();  
	Bitu group=(decode.modrm.val >> 3) & 7;
	if (decode.modrm.val >= 0xc0) { 
		dyn_fpu_top();
		switch(group){
		case 0x00:	/* FADD STi,ST*/
			gen_call_function((void*)&FPU_FADD,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x01:	/* FMUL STi,ST*/
			gen_call_function((void*)&FPU_FMUL,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x02:  /* FCOM*/
			gen_call_function((void*)&FPU_FCOM,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x03:  /* FCOMP*/
			gen_call_function((void*)&FPU_FCOM,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		case 0x04:  /* FSUBR STi,ST*/
			gen_call_function((void*)&FPU_FSUBR,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x05:  /* FSUB  STi,ST*/
			gen_call_function((void*)&FPU_FSUB,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x06:  /* FDIVR STi,ST*/
			gen_call_function((void*)&FPU_FDIVR,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x07:  /* FDIV STi,ST*/
			gen_call_function((void*)&FPU_FDIV,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		default:
			break;
		}
	} else { 
		dyn_fill_ea(); 
		gen_call_function((void*)&FPU_FLD_F64_EA,"%Ddr",DREG(EA)); 
		gen_load_host(&TOP,DREG(TMPB),4); 
		dyn_eatree();
	}
}

static void dyn_fpu_esc5(){
	dyn_get_modrm();  
	Bitu group=(decode.modrm.val >> 3) & 7;
	Bitu sub=(decode.modrm.val & 7);
	if (decode.modrm.val >= 0xc0) { 
		dyn_fpu_top();
		switch(group){
		case 0x00: /* FFREE STi */
			gen_call_function((void*)&FPU_FFREE,"%Ddr",DREG(EA));
			break;
		case 0x01: /* FXCH STi*/
			gen_call_function((void*)&FPU_FXCH,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x02: /* FST STi */
			gen_call_function((void*)&FPU_FST,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x03:  /* FSTP STi*/
			gen_call_function((void*)&FPU_FST,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		case 0x04:	/* FUCOM STi */
			gen_call_function((void*)&FPU_FUCOM,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x05:	/*FUCOMP STi */
			gen_call_function((void*)&FPU_FUCOM,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		default:
			LOG(LOG_FPU,LOG_WARN)("ESC 5:Unhandled group %d subfunction %d",group,sub);
			break;
        }
		gen_releasereg(DREG(EA));
		gen_releasereg(DREG(TMPB));
	} else {
		dyn_fill_ea(); 
		switch(group){
		case 0x00:  /* FLD double real*/
			gen_call_function((void*)&FPU_PREP_PUSH,"");
			gen_protectflags(); 
			gen_load_host(&TOP,DREG(TMPB),4); 
			gen_call_function((void*)&FPU_FLD_F64,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x01:  /* FISTTP longint*/
			LOG(LOG_FPU,LOG_WARN)("ESC 5 EA:Unhandled group %d subfunction %d",group,sub);
			break;
		case 0x02:   /* FST double real*/
			gen_call_function((void*)&FPU_FST_F64,"%Ddr",DREG(EA));
			break;
		case 0x03:	/* FSTP double real*/
			gen_call_function((void*)&FPU_FST_F64,"%Ddr",DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		case 0x04:	/* FRSTOR */
			gen_call_function((void*)&FPU_FRSTOR,"%Ddr",DREG(EA));
			break;
		case 0x06:	/* FSAVE */
			gen_call_function((void*)&FPU_FSAVE,"%Ddr",DREG(EA));
			break;
		case 0x07:   /*FNSTSW */
			gen_protectflags(); 
			gen_load_host(&TOP,DREG(TMPB),4); 
			gen_call_function((void*)&FPU_SET_TOP,"%Dd",DREG(TMPB));
			gen_load_host(&fpu.sw,DREG(TMPB),4); 
			gen_call_function((void*)&mem_writew,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		default:
			LOG(LOG_FPU,LOG_WARN)("ESC 5 EA:Unhandled group %d subfunction %d",group,sub);
		}
	}
}

static void dyn_fpu_esc6(){
	dyn_get_modrm();  
	Bitu group=(decode.modrm.val >> 3) & 7;
	Bitu sub=(decode.modrm.val & 7);
	if (decode.modrm.val >= 0xc0) { 
		dyn_fpu_top();
		switch(group){
		case 0x00:	/*FADDP STi,ST*/
			gen_call_function((void*)&FPU_FADD,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x01:	/* FMULP STi,ST*/
			gen_call_function((void*)&FPU_FMUL,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x02:  /* FCOMP5*/
			gen_call_function((void*)&FPU_FCOM,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;	/* TODO IS THIS ALLRIGHT ????????? */
		case 0x03:  /*FCOMPP*/
			if(sub != 1) {
				LOG(LOG_FPU,LOG_WARN)("ESC 6:Unhandled group %d subfunction %d",group,sub);
				return;
			}
			gen_load_host(&TOP,DREG(EA),4); 
			gen_dop_word_imm(DOP_ADD,true,DREG(EA),1);
			gen_dop_word_imm(DOP_AND,true,DREG(EA),7);
			gen_call_function((void*)&FPU_FCOM,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			gen_call_function((void*)&FPU_FPOP,""); /* extra pop at the bottom*/
			break;
		case 0x04:  /* FSUBRP STi,ST*/
			gen_call_function((void*)&FPU_FSUBR,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x05:  /* FSUBP  STi,ST*/
			gen_call_function((void*)&FPU_FSUB,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x06:	/* FDIVRP STi,ST*/
			gen_call_function((void*)&FPU_FDIVR,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x07:  /* FDIVP STi,ST*/
			gen_call_function((void*)&FPU_FDIV,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		default:
			break;
		}
		gen_call_function((void*)&FPU_FPOP,"");		
	} else {
		dyn_fill_ea(); 
		gen_call_function((void*)&FPU_FLD_I16_EA,"%Ddr",DREG(EA)); 
		gen_load_host(&TOP,DREG(TMPB),4); 
		dyn_eatree();
	}
}

static void dyn_fpu_esc7(){
	dyn_get_modrm();  
	Bitu group=(decode.modrm.val >> 3) & 7;
	Bitu sub=(decode.modrm.val & 7);
	if (decode.modrm.val >= 0xc0) { 
		switch (group){
		case 0x00: /* FFREEP STi*/
			dyn_fpu_top();
			gen_call_function((void*)&FPU_FFREE,"%Ddr",DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		case 0x01: /* FXCH STi*/
			dyn_fpu_top();
			gen_call_function((void*)&FPU_FXCH,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			break;
		case 0x02:  /* FSTP STi*/
		case 0x03:  /* FSTP STi*/
			dyn_fpu_top();
			gen_call_function((void*)&FPU_FST,"%Ddr%Ddr",DREG(TMPB),DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		case 0x04:
			switch(sub){
				case 0x00:     /* FNSTSW AX*/
					gen_load_host(&TOP,DREG(TMPB),4);
					gen_call_function((void*)&FPU_SET_TOP,"%Ddr",DREG(TMPB)); 
					gen_mov_host(&fpu.sw,DREG(EAX),2);
					break;
				default:
					LOG(LOG_FPU,LOG_WARN)("ESC 7:Unhandled group %d subfunction %d",group,sub);
					break;
			}
			break;
		default:
			LOG(LOG_FPU,LOG_WARN)("ESC 7:Unhandled group %d subfunction %d",group,sub);
			break;
		}
	} else {
		dyn_fill_ea(); 
		switch(group){
		case 0x00:  /* FILD Bit16s */
			gen_call_function((void*)&FPU_PREP_PUSH,"");
			gen_load_host(&TOP,DREG(TMPB),4); 
			gen_call_function((void*)&FPU_FLD_I16,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x01:
			LOG(LOG_FPU,LOG_WARN)("ESC 7 EA:Unhandled group %d subfunction %d",group,sub);
			break;
		case 0x02:   /* FIST Bit16s */
			gen_call_function((void*)&FPU_FST_I16,"%Ddr",DREG(EA));
			break;
		case 0x03:	/* FISTP Bit16s */
			gen_call_function((void*)&FPU_FST_I16,"%Ddr",DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		case 0x04:   /* FBLD packed BCD */
			gen_call_function((void*)&FPU_PREP_PUSH,"");
			gen_load_host(&TOP,DREG(TMPB),4);
			gen_call_function((void*)&FPU_FBLD,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x05:  /* FILD Bit64s */
			gen_call_function((void*)&FPU_PREP_PUSH,"");
			gen_load_host(&TOP,DREG(TMPB),4);
			gen_call_function((void*)&FPU_FLD_I64,"%Ddr%Ddr",DREG(EA),DREG(TMPB));
			break;
		case 0x06:	/* FBSTP packed BCD */
			gen_call_function((void*)&FPU_FBST,"%Ddr",DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		case 0x07:  /* FISTP Bit64s */
			gen_call_function((void*)&FPU_FST_I64,"%Ddr",DREG(EA));
			gen_call_function((void*)&FPU_FPOP,"");
			break;
		default:
			LOG(LOG_FPU,LOG_WARN)("ESC 7 EA:Unhandled group %d subfunction %d",group,sub);
			break;
		}
	}
}

#endif
