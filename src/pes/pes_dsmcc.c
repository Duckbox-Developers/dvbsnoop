/*
$Id: pes_dsmcc.c,v 1.1 2003/12/17 23:15:04 rasc Exp $



 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de



 -- PES  DSM-CC   ITU H.222.0  Annex B



$Log: pes_dsmcc.c,v $
Revision 1.1  2003/12/17 23:15:04  rasc
PES DSM-CC  ack and control commands  according ITU H.222.0 Annex B



*/




#include "dvbsnoop.h"
#include "strings/dsmcc_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"

#include "pes_dsmcc.h"



static  void dsmcc_control (u_char *b, int len);
static  void dsmcc_ack (u_char *b, int len);
static  int  dsmcc_timecode (u_char *b);






/*
   -- DSM-CC Command stream in a PES Packet!
   -- buffer starts with command_id

*/


void  PES_decodeDSMCC (u_char *b, int len)

{
 /* ITU H.222.0 Annex B */


   int   commandID;

   // -- already processed:
   // --- packet_start_code_prefix 	24 bslbf
   // --- stream_id 			8 uimsbf
   // --- packet_length			24 uimsbf

   commandID = outBit_S2x_NL (4,"Command_ID: ",	b,0,8,
			(char *(*)(u_int)) dsmccStr_Command_ID);
   b++;
   len--;

   if (commandID == 0x01) {

	dsmcc_control (b,len);

   } else if (commandID == 0x02) {

	dsmcc_ack (b,len);

   }


}





static void dsmcc_control (u_char *b, int len)
{
  int   select_flag;
  int   retrieval_flag;
  int   storage_flag;



   out_nl (3,"DSM-CC CONTROL: ");

   select_flag 		= outBit_Sx_NL (4,"select_flag: ",		b,0,1);
   retrieval_flag 	= outBit_Sx_NL (4,"retrieval_flag: ",		b,1,1);
   storage_flag 	= outBit_Sx_NL (4,"storage_flag: ",		b,2,1);

   outBit_Sx_NL (6,"reserved: ",		b,3,12);
   outBit_Sx_NL (4,"marker_bit: ",		b,15,1);

   b += 2;
//   len -= 2;


   if (select_flag) {
	u_long bitstreamID_31_17;
	u_long bitstreamID_16_2;
	u_long bitstreamID_1_0;

	out_nl (4,"SELECT:");
	indent (+1);

   		bitstreamID_31_17 = outBit_Sx_NL (5,"bitstream_id[31..17]: ",	b,0,15);
   				    outBit_Sx_NL (5,"marker_bit: ",		b,15,1);
   		bitstreamID_16_2  = outBit_Sx_NL (5,"bitstream_id[16..2]:  ",	b,16,15);
   				    outBit_Sx_NL (5,"marker_bit: ",		b,31,1);
   		bitstreamID_1_0   = outBit_Sx_NL (5,"bitstream_id[1..0]:   ",	b,32,2);
   		out_SL_NL (4," ==> bitstream_id:   ",
			(bitstreamID_31_17<<17) + (bitstreamID_16_2<<2) + bitstreamID_1_0);

		outBit_S2x_NL (4,"select_mode: ",	b,34,5,
				(char *(*)(u_int))dsmccStr_SelectMode_ID );
		outBit_Sx_NL (4,"marker_bit: ",		b,39,1);

	indent (-1);
	b += 5;
//	len -= 5;
   }


   if (retrieval_flag) {
	int  jump_flag;
	int  play_flag;

	out_nl (4,"RETRIEVE:");
	indent (+1);

	   	jump_flag	= outBit_Sx_NL (4,"jump_flag: ",	b,0,1);
   		play_flag	= outBit_Sx_NL (4,"play_flag: ",	b,1,1);

   		outBit_Sx_NL (4,"pause_mode: ",		b,2,1);
   		outBit_Sx_NL (4,"resume_mode: ",	b,3,1);
   		outBit_Sx_NL (4,"stop_mode: ",		b,4,1);
   		outBit_Sx_NL (6,"reserved: ",		b,5,10);
		outBit_Sx_NL (4,"marker_bit: ",		b,15,1);
		b += 2;
		len -= 2;

		if (jump_flag) {
		   int xlen = 0; 
		   out_nl (4,"JUMP:");
		   indent (+1);
   			outBit_Sx_NL  (6,"reserved: ",		b,0,7);
			outBit_S2x_NL (4,"direction_indicator: ",	b,7,1,
				(char *(*)(u_int)) dsmccStr_DirectionIndicator );
			indent (+1);
				xlen = dsmcc_timecode (b+1);
			b += xlen+1;
//			len -= (xlen+1);
		   indent (-2);
		}

		if (play_flag) {
		   int xlen; 
		   out_nl (4,"PLAY:");
		   indent (+1);
   		   	outBit_Sx_NL  (4,"speed_mode: ",			b,0,1);
   		   	outBit_S2x_NL (4,"direction_indicator: ",	b,1,1,
				(char *(*)(u_int)) dsmccStr_DirectionIndicator );
   		   	outBit_Sx_NL (6,"reserved: ",			b,2,6);
		   	indent (+1);
		   		xlen = dsmcc_timecode (b+1);
		   	b += xlen+1;
//			len -= (xlen+1);
		   indent (-2);
		}

	indent (-1);
   }



   if (storage_flag) {
	int  record_flag;

	out_nl (4,"STORAGE:");
	indent (+1);

		              outBit_Sx_NL (6,"reserved: ",	b,0,6);
		record_flag = outBit_Sx_NL (4,"record_flag: ",	b,6,1);
		              outBit_Sx_NL (4,"stop_mode: ",	b,7,1);
		b++;
//		len--;

		if (record_flag) {
			int  xlen;
			out_nl (4,"RECORD:");
			indent (+1);
			   	xlen = dsmcc_timecode (b);
			   	b += xlen;
//				len -= xlen;
			indent (-1);
		}

	indent (-1);
   }


}





 /* ITU H.222.0 Annex B */

static void dsmcc_ack (u_char *b, int len)
{
   int  select_ack;
   int  retrieval_ack;
   int  storage_ack;
   int  error_ack;
   int  cmd_status;


   out_nl (3,"DSM-CC  Acknowledge: ");

   select_ack 		= outBit_Sx_NL (4,"select_ack: ",	b,0,1);
   retrieval_ack 	= outBit_Sx_NL (4,"retrieval_ack: ",	b,1,1);
   storage_ack 		= outBit_Sx_NL (4,"storage_ack: ",	b,2,1);
   error_ack 		= outBit_Sx_NL (4,"error_ack: ",	b,3,1);
		          outBit_Sx_NL (6,"reserved: ",		b,4,10);
			  outBit_Sx_NL (4,"marker_bit: ",	b,14,1);
   cmd_status 		= outBit_Sx_NL (4,"cmd_status: ",	b,15,1);


   if (cmd_status && (retrieval_ack || storage_ack) ) {
	dsmcc_timecode (b+2);
   }

}






 /* ITU H.222.0 Annex B */

static int  dsmcc_timecode (u_char *b)
{
	int len = 0;
	int infinite_time_flag;

	outBit_Sx_NL (6,"reserved: ",		b,0,7);
	infinite_time_flag = outBit_Sx_NL (4,"infinite_time_flag: ",	b,7,1);
	b++;
	len = 1;

	if (infinite_time_flag == 0) {
		u_long   PTS_32_30;
		u_long   PTS_29_15;
		u_long   PTS_14_0;

		out_nl (4,"TIME PERIOD:");
		indent (+1);
		            outBit_Sx_NL (6,"reserved: ",	b,0,4);
   		PTS_32_30 = outBit_Sx_NL (5,"PTS[32..30]: ",	b,4,3);
		            outBit_Sx_NL (5,"marker_bit: ",	b,7,1);
   		PTS_29_15 = outBit_Sx_NL (5,"PTS[29..15]: ",	b,8,15);
		            outBit_Sx_NL (5,"marker_bit: ",	b,23,1);
   		PTS_14_0  = outBit_Sx_NL (5,"PTS[14..0]: ",	b,24,15);
		            outBit_Sx_NL (5,"marker_bit: ",	b,39,1);

		out_SL (4," ==> PTS = ",
			(PTS_32_30<<30) + (PTS_29_15<<15) + PTS_14_0);
			out_nl (4, " (cycles of the 90 kHz system clock)");

		indent (-1);
		len += 5;
	}

	return len;
}






  /* $$$ TODO some mode && cmd_status could be more verbose */

