/*
$Id: dsmcc_misc.c,v 1.1 2003/12/26 23:27:39 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de




$Log: dsmcc_misc.c,v $
Revision 1.1  2003/12/26 23:27:39  rasc
DSM-CC  UNT section




*/




#include "dvbsnoop.h"
#include "dsmcc_misc.h"
#include "strings/dvb_str.h"
#include "strings/dsmcc_str.h"
#include "descriptors/dsm_int_unt_descriptor.h"
#include "misc/output.h"
#include "misc/hexprint.h"



static int subDescriptor (u_char *b);





/*
 * -- dsmcc descriptor loops  (name, buffer)
 * --- P latform_descriptors
 * --- T arget_descriptors
 * --- O perational descriptors
 * -- return: len
 */

int dsmcc_pto_descriptor_loop (u_char *name, u_char *b)
{
   int loop_length;
   int len,i;


   out_nl (3,"%s_descriptor_loop:",name);
   indent (+1);

     outBit_Sx_NL (6,"reserved: ",		b,0,4);

     out (4,name);
     loop_length = outBit_Sx_NL (4,"_loop_length: ",	b,4,12);
     len = loop_length;
     b += 2;

     indent (+1);
     while (len > 0) {
	 i   = descriptorDSMCC_INT_UNT_Private (b);
	 b   += i;
	 len -= i;
	
     }

   indent (-2);
   return  (loop_length +2);
}






/*
 * ETSI TS 102 006 V1.2.1 (2002-10)
 * ISO/IEC 13818-6
 * This is a special descriptor loop
 */

int dsmcc_CompatibilityDescriptor(u_char *b)
{
   int  len;
   int  count;


   out_nl (3,"DSMCC_Compatibility Descriptor (loop):");
   indent (+1);
   len   = outBit_Sx_NL (4,"compatibilityDescriptorLength: ",	b, 0,16);
   count = outBit_Sx_NL (4,"DescriptorCount: ",			b,16,16);
   b += 4;
   len -= 4;


   while (count-- > 0) {
	int  subDesc_count;

	out_nl (4,"Descriptor (loop):");
	indent (+1);

   	outBit_S2x_NL (4,"descriptorType: ",		b, 0, 8,
			(char *(*)(u_int))dsmccStr_DescriptorType );
   	outBit_Sx_NL (4,"descriptorLength: ",		b, 8, 8);

   	outBit_S2x_NL (4,"specifierType: ",		b,16, 8,
			(char *(*)(u_int))dsmccStr_SpecifierType );
   	outBit_S2x_NL (4,"specifierData: ",		b,24,24,
			(char *(*)(u_int))dsmccStrOUI );
   	outBit_Sx_NL (4,"Model: ",			b,48,16);
   	outBit_Sx_NL (4,"Version: ",			b,64,16);

   	subDesc_count = outBit_Sx_NL (4,"SubDescriptorCount: ", b,80, 8);
	b    += 11;

	while (subDesc_count > 0) {
		int  i;

		out_nl (5,"SubDescriptor (loop):");
		indent (+1);
		i = subDescriptor (b);
   		indent (-1);
	}
   	indent (-1);
   }


   indent (-1);
   return len;
}





static int subDescriptor (u_char *b)

{
  int len;

  outBit_Sx_NL (5,"SubDescriptorType: ", 	b, 0, 8); 
  len = outBit_Sx_NL (5,"SubDescriptorlength: ",b, 8, 8);

  out_nl (4,"Additional Information:");
  indent (+1);
  printhexdump_buf (4, b+2, len);
  indent (-1);

  return len + 2;
}



