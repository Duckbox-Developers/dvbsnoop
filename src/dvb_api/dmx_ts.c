/*
$Id: dmx_ts.c,v 1.19 2004/01/25 21:37:28 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)


 -- Transport Streams
 --  For more information please see:
 --  ISO 13818 (-1) and ETSI 300 468




$Log: dmx_ts.c,v $
Revision 1.19  2004/01/25 21:37:28  rasc
bugfixes, minor changes & enhancments

Revision 1.18  2004/01/02 02:45:33  rasc
no message

Revision 1.17  2004/01/02 00:00:37  rasc
error output for buffer overflow

Revision 1.16  2004/01/01 20:09:23  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.15  2003/12/30 14:05:37  rasc
just some annotations, so I do not forget these over Sylvester party...
(some alkohol may reformat parts of /devbrain/0 ... )
cheers!

Revision 1.14  2003/12/28 22:53:40  rasc
some minor changes/cleanup

Revision 1.13  2003/12/28 14:00:25  rasc
bugfix: section read from input file
some changes on packet header output

Revision 1.12  2003/12/15 20:09:48  rasc
no message

Revision 1.11  2003/12/10 22:54:11  obi
more tiny fixes

Revision 1.10  2003/11/24 23:52:16  rasc
-sync option, some TS and PES stuff;
dsm_addr inactive, may be wrong - due to missing ISO 13818-6

Revision 1.9  2003/10/24 22:45:06  rasc
code reorg...

Revision 1.8  2003/10/24 22:17:18  rasc
code reorg...

Revision 1.7  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.6  2003/05/28 01:35:01  obi
fixed read() return code handling

Revision 1.5  2003/01/07 00:43:58  obi
set buffer size to 256kb

Revision 1.4  2002/11/01 20:38:40  Jolt
Changes for the new API

Revision 1.3  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "misc/cmdline.h"
#include "misc/output.h"
#include "misc/hexprint.h"
#include "misc/print_header.h"

#include "ts/tslayer.h"
#include "dvb_api.h"
#include "dmx_error.h"
#include "dmx_ts.h"



#define TS_BUF_SIZE  (256 * 1024)
#define TS_PACKET_LEN (188)              /* TS RDSIZE is fixed !! */
#define TS_SYNC_BYTE  (0x47)             /* SyncByte fuer TS  ISO 138181-1 */
#define READ_BUF_SIZE (3*TS_PACKET_LEN)	 /* min. 2x TS_PACKET_LEN!!! */



// $$ static long ts_SyncRead (int fd, u_char *buf, long max_buflen, long *skipped_bytes);
static long ts_SyncRead2 (int fd, u_char *buf, long max_buflen, long *skipped_bytes);





int  doReadTS (OPTION *opt)

{
  int     fd_dmx = 0, fd_dvr = 0;
  u_char  buf[READ_BUF_SIZE]; 	/* data buffer */
  u_char  *b;			/* ptr for packet start */
  long    count;
  int     i;
  char    *f;
  int     fileMode;


  

  if (opt->inpPidFile) {
  	f        = opt->inpPidFile;
        fileMode  = 1;
  } else {
  	f        = opt->devDvr;
        fileMode  = 0;
  } 


  if((fd_dvr = open(f,O_RDONLY)) < 0){
      IO_error(f);
      return -1;
  }


  


  /*
   -- init demux
  */

  if (!fileMode) {
    struct dmx_pes_filter_params flt;

    if((fd_dmx = open(opt->devDemux,O_RDWR)) < 0){
        IO_error(opt->devDemux);
	close (fd_dvr);
        return -1;
    }


    ioctl (fd_dmx,DMX_SET_BUFFER_SIZE, TS_BUF_SIZE);
    memset (&flt, 0, sizeof (struct dmx_pes_filter_params));

    flt.pid = opt->pid;
    flt.input  = DMX_IN_FRONTEND;
    flt.output = DMX_OUT_TS_TAP;
    flt.pes_type = DMX_PES_OTHER;
    flt.flags = DMX_IMMEDIATE_START;

    if ((i=ioctl(fd_dmx,DMX_SET_PES_FILTER,&flt)) < 0) {
      IO_error ("DMX_SET_PES_FILTER failed: ");
      return -1;
    }

}




/*
  -- read TS packets for pid
*/

  count = 0;
  while (1) {
    long   n;
    long   skipped_bytes = 0;


    if (opt->packet_header_sync) {
    	// $$ n = ts_SyncRead (fd_dvr,buf,sizeof(buf), &skipped_bytes);
	// $$ b = buf;
    	n = ts_SyncRead2 (fd_dvr,buf,sizeof(buf), &skipped_bytes);
        b = buf+(skipped_bytes % TS_PACKET_LEN);
    } else {
    	n = read(fd_dvr,buf,TS_PACKET_LEN);
	b = buf;
    }


    // -- error or eof?
    if (n == -1) IO_error("read");
    if (n < 0)  continue;
    if (n == 0) {
	if (!fileMode) continue;	// DVRmode = no eof!
	else break;			// filemode eof 
    }


    count ++;

    if (opt->binary_out) {

       // direct write to FD 1 ( == stdout)
       write (1, b, n);

    } else {

       indent (0);
       print_packet_header (opt, "TS", opt->pid, count, n, skipped_bytes);


       if (opt->printhex) {
           printhex_buf (0, b, n);
           out_NL(0);
       }


       // decode protocol
       if (opt->printdecode) {
          decodeTS_buf (b, n ,opt->pid);
          out_nl (3,"==========================================================");
          out_NL (3);
       }
    } // bin_out



    // Clean Buffer
//    if (n > 0 && n < sizeof(b)) memset (buf,0,n+1); 


    // count packets ?
    if (opt->packet_count > 0) {
       if (--opt->packet_count == 0) break;
    }


  } // while



  /*
    -- Stop Demux
  */

  if (!fileMode) {
     ioctl (fd_dmx, DMX_STOP, 0);

     close(fd_dmx);
  }

  close(fd_dvr);
  return 0;
}



/*
 * -- sync read (optimized = avoid multiple byte reads)
 * -- Seek TS sync-byte and read packet in buffer
 * -- ATTENTION:  packet is not stored at buf[0]
 *  --       -->  packet starts at buf[skipped_bytes % TS_PACKET_LEN] !!!
 * -- return: equivalent to read();
 */

static long  ts_SyncRead2 (int fd, u_char *buf, long max_buflen, long *skipped_bytes)

{
    int    n1,n2;
    int    i;
    int    found;


    // -- simple TS sync...
    // -- $$$ to be improved:
    // -- $$$  (best would be: check if buf[188] is also a sync byte)
 

    *skipped_bytes = 0;
    found = 0;
    while (! found) {
    	n1 = read(fd,buf,TS_PACKET_LEN);
    	if (n1 <= 0) return n1;			// error or strange, abort

    	for (i=0;i<n1; i++) {			// search sync byte
		if (buf[i] == TS_SYNC_BYTE) {
			found = 1;
			break;
		}
    	}
    	*skipped_bytes += i;
    }

    // -- Sync found!
    // -- read skipped number of bytes per read try

    if (i == 0) return n1;			// already complete packet read...

    n2 = read(fd,buf+n1,TS_PACKET_LEN-n1+i);	// continue read TS packet
    if (n2 >=0) n2 = n1+n2-i; ;			// should be TS_PACKET_LEN anyway...

    return n2;
}





#if 0
// $$ old routine....

/*
 * -- sync read
 * -- Seek TS sync-byte and read buffer
 * -- return: equivalent to read();
 */

static long  ts_SyncRead (int fd, u_char *buf, long buflen, long *skipped_bytes)

{
    int    n = 0;


    // -- simple TS sync...
    // -- $$$ to be improved:
    // -- $$$  (best would be: check if buf[188] is also a sync byte)
 
    // -- $$$ todo  speed optimize! (read TS_PACKET_LEN, seek Sync, read missing parts)

    *skipped_bytes = 0;
    while (1) {
    	n = read(fd,buf,1);
	if (n <= 0) return n;			// error or strange, abort

	if (buf[0] == TS_SYNC_BYTE) break;
	(*skipped_bytes)++;			// sync skip counter
    }

    // -- Sync found!
    // -- read buffersize-1

    n = read(fd,buf+1,TS_PACKET_LEN-1);		// read TS
    if (n >=0) n++;				// we already read one byte...

    return n;
}

#endif







