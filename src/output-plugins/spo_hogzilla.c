/*
** Copyright (C) 2015-2015 Hogzilla <dev@ids-hogzilla.org>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* $Id$ */

/* spo_hogzilla
 * 
 * Purpose:
 *
 * TODO: HZ
 * This plugin generates 
 *
 * Arguments:
 *   
 * filename of the output log (default: snort.log)
 *
 * Effect:
 *
 * Packet logs are written (quickly) to a tcpdump formatted output
 * file
 *
 * Comments:
 *
 * First logger...
 *
 */

 #define HOGZILLA_MAX_NDPI_FLOWS 500
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <sys/types.h>
// #include <ctype.h>
// #include <stdlib.h>
// #include <string.h>
// #include <errno.h>
// #include <unistd.h>
// #include <time.h>
// 
// #include "decode.h"
// #include "mstring.h"
// #include "plugbase.h"
// #include "parser.h"
// #include "debug.h"
// #include "util.h"
// #include "map.h"
// #include "unified2.h"
// 
// #include "barnyard2.h"
//

// #include <sched.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <netinet/in.h>
// #include <string.h>
// #include <stdarg.h>
// #include <search.h>
// #include <signal.h>
#include "ndpi_api.h"
// #include <sys/socket.h>


// TODO HZ
// struct
typedef struct _HogzillaData
{
    char                *filename;
    pcap_t              *pd;	       /* pcap handle */
    pcap_dumper_t       *dumpd;
    time_t              lastTime;
    size_t              size;
    size_t              limit;
    char                logdir[STD_BUF];

    int                 autolink;
    int                 linktype;
} HogzillaData;

//CSP
// - struct reader_thread  
// - declaração ndpi_thread_info
// extraido de ndpiReader.c
//PSC


struct reader_thread {
  struct ndpi_detection_module_struct *ndpi_struct;
  void *ndpi_flows_root[NUM_ROOTS];
  char _pcap_error_buffer[PCAP_ERRBUF_SIZE];
  pcap_t *_pcap_handle;
  u_int64_t last_time;
  u_int64_t last_idle_scan_time;
  u_int32_t idle_scan_idx;
  u_int32_t num_idle_flows;
  pthread_t pthread;
  int _pcap_datalink_type;

  /* TODO Add barrier */
  struct thread_stats stats;

  struct ndpi_flow *idle_flows[IDLE_SCAN_BUDGET];
};

static struct reader_thread ndpi_thread_info[MAX_NUM_READER_THREADS];

/* list of function prototypes for this output plugin */
// TODO HZ
// atualizar essa lista no final
static void HogzillaInit(char *);
static HogzillaData *ParseHogzillaArgs(char *);
static void Hogzilla(Packet *, void *, uint32_t, void *);
static void SpoHogzillaCleanExitFunc(int, void *);
static void SpoHogzillaRestartFunc(int, void *);
static void HogzillaSingle(Packet *, void *, uint32_t, void *);
static void HogzillaStream(Packet *, void *, uint32_t, void *);
//static void HogzillaInitLogFileFinalize(int unused, void *arg);
//static void HogzillaInitLogFile(HogzillaData *, int);
//static void HogzillaRollLogFile(HogzillaData*);


/* If you need to instantiate the plugin's data structure, do it here */
HogzillaData *hogzilla_ptr;

/*
 * Function: HogzillaSetup()
 *
 * Purpose: Registers the output plugin keyword and initialization 
 *          function into the output plugin list.  This is the function that
 *          gets called from InitOutputPlugins() in plugbase.c.
 *
 * Arguments: None.
 *
 * Returns: void function
 *
 */
void HogzillaSetup(void)
{
    /* link the preprocessor keyword to the init function in 
       the preproc list */
    RegisterOutputPlugin("hogzilla", OUTPUT_TYPE_FLAG__LOG, HogzillaInit);

    DEBUG_WRAP(DebugMessage(DEBUG_INIT,"Output plugin: Hogzilla is setup...\n"););
}


/*
 * Function: HogzillaInit(char *)
 *
 * Purpose: Calls the argument parsing function, performs final setup on data
 *          structs, links the preproc function into the function list.
 *
 * Arguments: args => ptr to argument string
 *
 * Returns: void function
 *
 */
static void HogzillaInit(char *args)
{
    HogzillaData *data;
    DEBUG_WRAP(DebugMessage(DEBUG_INIT,"Output: Hogzilla Initialized\n"););

    /* parse the argument list from the rules file */
    data = ParseHogzillaArgs(args);
    hogzilla_ptr = data;

    //AddFuncToPostConfigList(HogzillaInitLogFileFinalize, data);
    //DEBUG_WRAP(DebugMessage(DEBUG_INIT,"Linking Hogzilla functions to call lists...\n"););
    
    /* Set the preprocessor function into the function list */
    AddFuncToOutputList(Hogzilla, OUTPUT_TYPE__LOG, data);
    AddFuncToCleanExitList(SpoHogzillaCleanExitFunc, data);
    AddFuncToRestartList(SpoHogzillaRestartFunc, data);
}

/*
 * Function: ParseHogzillaArgs(char *)
 *
 * Purpose: Process positional args, if any.  Syntax is:
 * TODO HZ
 * output log_tcpdump: [<logpath> [<limit>]]
 * limit ::= <number>('G'|'M'|K')
 *
 * Arguments: args => argument list
 *
 * Returns: void function
 */
static HogzillaData *ParseHogzillaArgs(char *args)
{
    char **toks;
    int num_toks;
    HogzillaData *data;
    int i;

    DEBUG_WRAP(DebugMessage(DEBUG_LOG, "ParseHogzillaArgs: %s\n", args););
    data = (HogzillaData *) SnortAlloc(sizeof(HogzillaData));

    if ( data == NULL )
    {
        FatalError("hogzilla: unable to allocate memory!\n");
    }
    data->filename = "localhost";

    if ( args == NULL )
        args = "";

    toks = mSplit((char*)args, " \t", 0, &num_toks, '\\');

    for (i = 0; i < num_toks; i++)
    {
        const char* tok = toks[i];
        char *end;

        switch (i)
        {
            case 0:
                data->filename = SnortStrdup(tok);
                break;

            case 1:
                data->limit = strtol(tok, &end, 10);

                if ( tok == end )
                    FatalError("log_tcpdump error in %s(%i): %s\n",
                        file_name, file_line, tok);

                if ( end && toupper(*end) == 'G' )
                    data->limit <<= 30; /* GB */

                else if ( end && toupper(*end) == 'M' )
                    data->limit <<= 20; /* MB */

                else if ( end && toupper(*end) == 'K' )
                    data->limit <<= 10; /* KB */
                break;

            case 2:

                if ( !strncmp(tok, "DLT_EN10MB", 10) )             /* ethernet */
                {
                    data->linktype = DLT_EN10MB;
                    data->autolink = 0;
                }
#ifdef DLT_IEEE802_11
                else if ( !strncmp(tok, "DLT_IEEE802_11", 14) )    
                {
                    data->linktype = DLT_IEEE802_11;
                    data->autolink = 0;
                }
#endif
#ifdef DLT_ENC
                else if ( !strncmp(tok, "DLT_ENC", 7) )            /* encapsulated data */
                {
                    data->linktype = DLT_ENC;
                    data->autolink = 0;
                }
#endif
                else if ( !strncmp(tok, "DLT_IEEE805", 11) )       /* token ring */
                {
                    data->linktype = DLT_IEEE802;
                    data->autolink = 0;
                }
                else if ( !strncmp(tok, "DLT_FDDI", 8) )           /* FDDI */
                {
                    data->linktype = DLT_FDDI;
                    data->autolink = 0;
                }
#ifdef DLT_CHDLC
                else if ( !strncmp(tok, "DLT_CHDLC", 9) )          /* cisco HDLC */
                {
                    data->linktype = DLT_CHDLC;
                    data->autolink = 0;
                }
#endif
                else if ( !strncmp(tok, "DLT_SLIP", 8) )           /* serial line internet protocol */
                {
                    data->linktype = DLT_SLIP;
                    data->autolink = 0;
                }
                else if ( !strncmp(tok, "DLT_PPP", 7) )            /* point-to-point protocol */
                {
                    data->linktype = DLT_PPP;
                    data->autolink = 0;
                }
#ifdef DLT_PPP_SERIAL
                else if ( !strncmp(tok, "DLT_PPP_SERIAL", 14) )     /* PPP with full HDLC header */
                {
                    data->linktype = DLT_PPP_SERIAL;
                    data->autolink = 0;
                }
#endif
#ifdef DLT_LINUX_SLL
                else if ( !strncmp(tok, "DLT_LINUX_SLL", 13) )     
                {
                    data->linktype = DLT_LINUX_SLL;
                    data->autolink = 0;
                }
#endif
#ifdef DLT_PFLOG
                else if ( !strncmp(tok, "DLT_PFLOG", 9) )     
                {
                    data->linktype = DLT_PFLOG;
                    data->autolink = 0;
                }
#endif
#ifdef DLT_OLDPFLOG
                else if ( !strncmp(tok, "DLT_OLDPFLOG", 12) )     
                {
                    data->linktype = DLT_OLDPFLOG;
                    data->autolink = 0;
                }
#endif

                break;

            case 3:
                FatalError("log_tcpdump: error in %s(%i): %s\n",
                    file_name, file_line, tok);
                break;
        }
    }
    mSplitFree(&toks, num_toks);

    if ( data->filename == NULL )
        data->filename = SnortStrdup(DEFAULT_FILE);

    DEBUG_WRAP(DebugMessage(
        DEBUG_INIT, "hogzilla: '%s' %ld\n", data->filename, data->limit
    ););
    return data;
}

/*
 * Function: PreprocFunction(Packet *)
 *
 * Purpose: Perform the preprocessor's intended function.  This can be
 *          simple (statistics collection) or complex (IP defragmentation)
 *          as you like.  Try not to destroy the performance of the whole
 *          system by trying to do too much....
 *
 * Arguments: p => pointer to the current packet data struct 
 *
 * Returns: void function
 */
static void Hogzilla(Packet *p, void *event, uint32_t event_type, void *arg)
{

// Processar na nDPI
//    . lá na nDPI, quando ocorrer uma das abaixo, o flow deve ser salvo no HBASE
//       i  ) Conexão terminou
//       ii ) Atingiu 500 pacotes no fluxo
//       iii) A conexão ficou IDLE por mais de MAX_IDLE_TIME

    if(p)
    {
        if(p->packet_flags & PKT_REBUILT_STREAM)
        {
            HogzillaStream(p, event, event_type, arg);
        }
        else
        {
            HogzillaSingle(p, event, event_type, arg);
        }
    }
}

static INLINE size_t SizeOf (const struct pcap_pkthdr *pkth)
{
    return PCAP_PKT_HDR_SZ + pkth->caplen;
}

static void HogzillaSingle(Packet *p, void *event, uint32_t event_type, void *arg)
{
    HogzillaData *data = (HogzillaData *)arg;
    size_t dumpSize = SizeOf(p->pkth);

    /* roll log file packet linktype is different to the dump linktype and in automode */

//    if ( data->linktype != p->linktype )
//    {
//        if ( data->autolink == 1)
//        {
//            data->linktype = p->linktype;
//            HogzillaRollLogFile(data);
//        }
        /* otherwise alert that the results will not be as expected */
//        else
//        {
//            LogMessage("tcpdump:  packet linktype is not compatible with dump linktype.\n");
//        }
//    }

    /* roll log file if size limit is exceeded */
//    else if ( data->size + dumpSize > data->limit )
//        HogzillaRollLogFile(data);

    pcap_dump((u_char *)data->dumpd, p->pkth, p->pkt);
    data->size += dumpSize;

    if (!BcLineBufferedLogging())
    { 
#ifdef WIN32
        fflush( NULL );  /* flush all open output streams */
#else
        /* we happen to know that pcap_dumper_t* is really just a FILE* */
        fflush( (FILE*) data->dumpd );
#endif
    }
}

static void HogzillaStream(Packet *p, void *event, uint32_t event_type, void *arg)
{
    HogzillaData *data = (HogzillaData *)arg;
    size_t dumpSize = 0;

//    if (stream_api)
//        stream_api->traverse_reassembled(p, SizeOfCallback, &dumpSize);

    if ( data->size + dumpSize > data->limit )
        HogzillaRollLogFile(data);

//    if (stream_api)
//        stream_api->traverse_reassembled(p, HogzillaStreamCallback, data);

    data->size += dumpSize;

    if (!BcLineBufferedLogging())
    { 
#ifdef WIN32
        fflush( NULL );  /* flush all open output streams */
#else
        /* we happen to know that pcap_dumper_t* is really just a FILE* */
        fflush( (FILE*) data->dumpd );
#endif
    }
}

static void HogzillaInitLogFileFinalize(int unused, void *arg)
{
    HogzillaInitLogFile((HogzillaData *)arg, BcNoOutputTimestamp());
}

/*
 * Function: HogzillaInitLogFile()
 *
 * Purpose: Initialize the tcpdump log file header
 *
 * Arguments: data => pointer to the plugin's reference data struct 
 *
 * Returns: void function
 */
static void HogzillaInitLogFile(HogzillaData *data, int nostamps)
{
    int value;
    data->lastTime = time(NULL);

    if (nostamps)
    {
        if(data->filename[0] == '/')
            value = SnortSnprintf(data->logdir, STD_BUF, "%s", data->filename);
        else
            value = SnortSnprintf(data->logdir, STD_BUF, "%s/%s", barnyard2_conf->log_dir, 
                                  data->filename);
    }
    else 
    {
        if(data->filename[0] == '/')
            value = SnortSnprintf(data->logdir, STD_BUF, "%s.%lu", data->filename, 
                                  (uint32_t)data->lastTime);
        else
            value = SnortSnprintf(data->logdir, STD_BUF, "%s/%s.%lu", barnyard2_conf->log_dir, 
                                  data->filename, (uint32_t)data->lastTime);
    }

    if(value != SNORT_SNPRINTF_SUCCESS)
        FatalError("log file logging path and file name are too long\n");

    DEBUG_WRAP(DebugMessage(DEBUG_LOG, "Opening %s\n", data->logdir););

    if(!BcTestMode())
    {
        data->pd = pcap_open_dead(data->linktype, PKT_SNAPLEN);
        data->dumpd = pcap_dump_open(data->pd, data->logdir);

        if(data->dumpd == NULL)
        {
            FatalError("log_tcpdump: Failed to open log file \"%s\": %s\n",
                       data->logdir, strerror(errno));
        }
    }

    data->size = PCAP_FILE_HDR_SZ;
}

static void HogzillaRollLogFile(HogzillaData* data)
{
    time_t now = time(NULL);

    /* don't roll over any sooner than resolution
     * of filename discriminator
     */
    if ( now <= data->lastTime ) return;

    /* close the output file */
    if( data->dumpd != NULL )
    {
        pcap_dump_close(data->dumpd);
        data->dumpd = NULL;
        data->size = 0;
    }

    /* close the pcap */
    if (data->pd != NULL)
    {
        pcap_close(data->pd);
        data->pd = NULL;
    }

    /* Have to add stamps now to distinguish files */
    HogzillaInitLogFile(data, 0);
}

/*
 * Function: SpoHogzillaCleanExitFunc()
 *
 * Purpose: Cleanup at exit time
 *
 * Arguments: signal => signal that caused this event
 *            arg => data ptr to reference this plugin's data
 *
 * Returns: void function
 */
static void SpoHogzillaCleanup(int signal, void *arg, const char* msg)
{
  // Fecha conexão no banco
  // limpar memoria ocupada pelo nDPI
  // Gera log
  
    /* cast the arg pointer to the proper type */
    HogzillaData *data = (HogzillaData *) arg;

    DEBUG_WRAP(DebugMessage(DEBUG_LOG,"%s\n", msg););

    /* close the output file */
    if( data->dumpd != NULL )
    {
        pcap_dump_close(data->dumpd);
        data->dumpd = NULL;
    }

    /* close the pcap */
    if (data->pd != NULL)
    {
        pcap_close(data->pd);
        data->pd = NULL;
    }

    /* 
     * if we haven't written any data, dump the output file so there aren't
     * fragments all over the disk 
     */
     /*
    if(!BcTestMode() && *data->logdir && pc.alert_pkts==0 && pc.log_pkts==0)
    {
        int ret;

        ret = unlink(data->logdir);

        if (ret != 0)
        {
            ErrorMessage("Could not remove tcpdump output file %s: %s\n",
                         data->logdir, strerror(errno));
        }
    }*/

    if (data->filename)
    {
        free (data->filename);
    }

    memset(data,'\0',sizeof(HogzillaData));
    free(data);
}

static void SpoHogzillaCleanExitFunc(int signal, void *arg)
{
    SpoHogzillaCleanup(signal, arg, "SpoHogzillaCleanExitFunc");
}

static void SpoHogzillaRestartFunc(int signal, void *arg)
{
    SpoHogzillaCleanup(signal, arg, "SpoHogzillaRestartFunc");
}

// CSP
// Observação: SpoHogzillaCleanExitFunc e SpoHogzillaRestartFunc fazem a mesma coisa, chamam a função SpoHogzillaCleanup
//				pode diferenciar na passagem de parametros!! Observar!!!
// PSC


// flow tracking
typedef struct ndpi_flow {
  u_int32_t lower_ip;
  u_int32_t upper_ip;
  u_int16_t lower_port;
  u_int16_t upper_port;
  u_int8_t detection_completed, protocol;
  u_int16_t vlan_id;
  struct ndpi_flow_struct *ndpi_flow;
  char lower_name[32], upper_name[32];

  u_int64_t last_seen;

  u_int64_t bytes;
  u_int32_t packets;
  //PA
  u_int32_t max_packet_size;
  u_int32_t min_packet_size;
  u_int32_t avg_packet_size;

  u_int64_t payload_bytes;
  u_int32_t payload_first_size;
  u_int32_t payload_avg_size;
  u_int32_t payload_min_size;
  u_int32_t payload_max_size;
  u_int32_t packets_without_payload;

  u_int64_t flow_duration;
  u_int64_t first_seen;

  u_int32_t inter_time[500];
  u_int64_t packet_size[500];
  //AP

  // result only, not used for flow identification
  ndpi_protocol detected_protocol;

  char host_server_name[256];

  struct {
    char client_certificate[48], server_certificate[48];
  } ssl;

  void *src_id, *dst_id;
} ndpi_flow_t;



/* ***************************************************** */
static void node_proto_guess_walker(const void *node, ndpi_VISIT which, int depth, void *user_data) {
  struct ndpi_flow *flow = *(struct ndpi_flow **) node;
  u_int16_t thread_id = *((u_int16_t *) user_data);

#if 0
  printf("<%d>Walk on node %s (%p)\n",
     depth,
     which == preorder?"preorder":
     which == postorder?"postorder":
     which == endorder?"endorder":
     which == leaf?"leaf": "unknown",
     flow);
#endif

  if((which == ndpi_preorder) || (which == ndpi_leaf)) { /* Avoid walking the same node multiple times */
    if(enable_protocol_guess) {
      if(flow->detected_protocol.protocol == NDPI_PROTOCOL_UNKNOWN) {
    node_guess_undetected_protocol(thread_id, flow);
    // printFlow(thread_id, flow);
      }
    }

    ndpi_thread_info[thread_id].stats.protocol_counter[flow->detected_protocol.protocol]       += flow->packets;
    ndpi_thread_info[thread_id].stats.protocol_counter_bytes[flow->detected_protocol.protocol] += flow->bytes;
    ndpi_thread_info[thread_id].stats.protocol_flows[flow->detected_protocol.protocol]++;
  }
}
/* ***************************************************** */

static void node_idle_scan_walker(const void *node, ndpi_VISIT which, int depth, void *user_data) {
  struct ndpi_flow *flow = *(struct ndpi_flow **) node;
  u_int16_t thread_id = *((u_int16_t *) user_data);
  
  
  //  Conexões idle, salva no HBASE e apaga

  if(ndpi_thread_info[thread_id].num_idle_flows == IDLE_SCAN_BUDGET) /* TODO optimise with a budget-based walk */
    return;

  if((which == ndpi_preorder) || (which == ndpi_leaf)) { /* Avoid walking the same node multiple times */
    if(flow->last_seen + MAX_IDLE_TIME < ndpi_thread_info[thread_id].last_time) {

      /* update stats */
      node_proto_guess_walker(node, which, depth, user_data);

      if((flow->detected_protocol.protocol == NDPI_PROTOCOL_UNKNOWN) && !undetected_flows_deleted)
        undetected_flows_deleted = 1;
      
      // TODO HZ: HogzillaSaveFlow 
	  // --- salvar no HBASE??
      free_ndpi_flow(flow);
      ndpi_thread_info[thread_id].stats.ndpi_flow_count--;

      /* adding to a queue (we can't delete it from the tree inline ) */
      ndpi_thread_info[thread_id].idle_flows[ndpi_thread_info[thread_id].num_idle_flows++] = flow;
    }
  }
}
/* ***************************************************** */

static int node_cmp(const void *a, const void *b) {
  struct ndpi_flow *fa = (struct ndpi_flow*)a;
  struct ndpi_flow *fb = (struct ndpi_flow*)b;

  if(fa->vlan_id   < fb->vlan_id  )   return(-1); else { if(fa->vlan_id   > fb->vlan_id  )   return(1); }
  if(fa->lower_ip   < fb->lower_ip  ) return(-1); else { if(fa->lower_ip   > fb->lower_ip  ) return(1); }
  if(fa->lower_port < fb->lower_port) return(-1); else { if(fa->lower_port > fb->lower_port) return(1); }
  if(fa->upper_ip   < fb->upper_ip  ) return(-1); else { if(fa->upper_ip   > fb->upper_ip  ) return(1); }
  if(fa->upper_port < fb->upper_port) return(-1); else { if(fa->upper_port > fb->upper_port) return(1); }
  if(fa->protocol   < fb->protocol  ) return(-1); else { if(fa->protocol   > fb->protocol  ) return(1); }

  return(0);
}

/* ***************************************************** */

static struct ndpi_flow *get_ndpi_flow(u_int16_t thread_id,
                       const u_int8_t version,
                       u_int16_t vlan_id,
                       const struct ndpi_iphdr *iph,
                       u_int16_t ip_offset,
                       u_int16_t ipsize,
                       u_int16_t l4_packet_len,
                       struct ndpi_id_struct **src,
                       struct ndpi_id_struct **dst,
                       u_int8_t *proto,
                       const struct ndpi_ip6_hdr *iph6) {
  u_int32_t idx, l4_offset;
  struct ndpi_tcphdr *tcph = NULL;
  struct ndpi_udphdr *udph = NULL;
  u_int32_t lower_ip;
  u_int32_t upper_ip;
  u_int16_t lower_port;
  u_int16_t upper_port;
  struct ndpi_flow flow;
  void *ret;
  u_int8_t *l3;

  /*
    Note: to keep things simple (ndpiReader is just a demo app)
    we handle IPv6 a-la-IPv4.
  */
  if(version == 4) {
    if(ipsize < 20)
      return NULL;

    if((iph->ihl * 4) > ipsize || ipsize < ntohs(iph->tot_len)
       || (iph->frag_off & htons(0x1FFF)) != 0)
      return NULL;

    l4_offset = iph->ihl * 4;
    l3 = (u_int8_t*)iph;
  } else {
    l4_offset = sizeof(struct ndpi_ip6_hdr);
    l3 = (u_int8_t*)iph6;
  }

  if(l4_packet_len < 64)
    ndpi_thread_info[thread_id].stats.packet_len[0]++;
  else if(l4_packet_len >= 64 && l4_packet_len < 128)
    ndpi_thread_info[thread_id].stats.packet_len[1]++;
  else if(l4_packet_len >= 128 && l4_packet_len < 256)
    ndpi_thread_info[thread_id].stats.packet_len[2]++;
  else if(l4_packet_len >= 256 && l4_packet_len < 1024)
    ndpi_thread_info[thread_id].stats.packet_len[3]++;
  else if(l4_packet_len >= 1024 && l4_packet_len < 1500)
    ndpi_thread_info[thread_id].stats.packet_len[4]++;
  else if(l4_packet_len >= 1500)
    ndpi_thread_info[thread_id].stats.packet_len[5]++;

  if(l4_packet_len > ndpi_thread_info[thread_id].stats.max_packet_len)
    ndpi_thread_info[thread_id].stats.max_packet_len = l4_packet_len;

  if(iph->saddr < iph->daddr) {
    lower_ip = iph->saddr;
    upper_ip = iph->daddr;
  } else {
    lower_ip = iph->daddr;
    upper_ip = iph->saddr;
  }

  *proto = iph->protocol;

  if(iph->protocol == 6 && l4_packet_len >= 20) {
    ndpi_thread_info[thread_id].stats.tcp_count++;

    // tcp
    tcph = (struct ndpi_tcphdr *) ((u_int8_t *) l3 + l4_offset);
    if(iph->saddr < iph->daddr) {
      lower_port = tcph->source;
      upper_port = tcph->dest;
    } else {
      lower_port = tcph->dest;
      upper_port = tcph->source;

      if(iph->saddr == iph->daddr) {
    if(lower_port > upper_port) {
      u_int16_t p = lower_port;

      lower_port = upper_port;
      upper_port = p;
    }
      }
    }
  } else if(iph->protocol == 17 && l4_packet_len >= 8) {
    // udp
    ndpi_thread_info[thread_id].stats.udp_count++;

    udph = (struct ndpi_udphdr *) ((u_int8_t *) l3 + l4_offset);
    if(iph->saddr < iph->daddr) {
      lower_port = udph->source;
      upper_port = udph->dest;
    } else {
      lower_port = udph->dest;
      upper_port = udph->source;
    }
  } else {
    // non tcp/udp protocols
    lower_port = 0;
    upper_port = 0;
  }

  flow.protocol = iph->protocol, flow.vlan_id = vlan_id;
  flow.lower_ip = lower_ip, flow.upper_ip = upper_ip;
  flow.lower_port = lower_port, flow.upper_port = upper_port;

//  if(0)
//    printf("[NDPI] [%u][%u:%u <-> %u:%u]\n",
//     iph->protocol, lower_ip, ntohs(lower_port), upper_ip, ntohs(upper_port));

  idx = (vlan_id + lower_ip + upper_ip + iph->protocol + lower_port + upper_port) % NUM_ROOTS;
  ret = ndpi_tfind(&flow, &ndpi_thread_info[thread_id].ndpi_flows_root[idx], node_cmp);

  if(ret == NULL) {
    if(ndpi_thread_info[thread_id].stats.ndpi_flow_count == MAX_NDPI_FLOWS) {
      printf("ERROR: maximum flow count (%u) has been exceeded\n", MAX_NDPI_FLOWS);
      exit(-1);
    } else {
      struct ndpi_flow *newflow = (struct ndpi_flow*)malloc(sizeof(struct ndpi_flow));

      if(newflow == NULL) {
    printf("[NDPI] %s(1): not enough memory\n", __FUNCTION__);
    return(NULL);
      }

      memset(newflow, 0, sizeof(struct ndpi_flow));
      newflow->protocol = iph->protocol, newflow->vlan_id = vlan_id;
      newflow->lower_ip = lower_ip, newflow->upper_ip = upper_ip;
      newflow->lower_port = lower_port, newflow->upper_port = upper_port;
      //PA NEWFLOW
      newflow->min_packet_size=999999;
      newflow->payload_min_size=999999;
      //AP

      if(version == 4) {
    inet_ntop(AF_INET, &lower_ip, newflow->lower_name, sizeof(newflow->lower_name));
    inet_ntop(AF_INET, &upper_ip, newflow->upper_name, sizeof(newflow->upper_name));
      } else {
    inet_ntop(AF_INET6, &iph6->ip6_src, newflow->lower_name, sizeof(newflow->lower_name));
    inet_ntop(AF_INET6, &iph6->ip6_dst, newflow->upper_name, sizeof(newflow->upper_name));
      }

      if((newflow->ndpi_flow = malloc_wrapper(size_flow_struct)) == NULL) {
    printf("[NDPI] %s(2): not enough memory\n", __FUNCTION__);
    free(newflow);
    return(NULL);
      } else
    memset(newflow->ndpi_flow, 0, size_flow_struct);
      if((newflow->src_id = malloc_wrapper(size_id_struct)) == NULL) {
    printf("[NDPI] %s(3): not enough memory\n", __FUNCTION__);
    free(newflow);
    return(NULL);
      } else
    memset(newflow->src_id, 0, size_id_struct);

      if((newflow->dst_id = malloc_wrapper(size_id_struct)) == NULL) {
    printf("[NDPI] %s(4): not enough memory\n", __FUNCTION__);
    free(newflow);
    return(NULL);
      } else
    memset(newflow->dst_id, 0, size_id_struct);

      ndpi_tsearch(newflow, &ndpi_thread_info[thread_id].ndpi_flows_root[idx], node_cmp); /* Add */
      ndpi_thread_info[thread_id].stats.ndpi_flow_count++;

      *src = newflow->src_id, *dst = newflow->dst_id;

      // printFlow(thread_id, newflow);

      return newflow ;
    }
  } else {
    struct ndpi_flow *flow = *(struct ndpi_flow**)ret;

    if(flow->lower_ip == lower_ip && flow->upper_ip == upper_ip
       && flow->lower_port == lower_port && flow->upper_port == upper_port)
      *src = flow->src_id, *dst = flow->dst_id;
    else
      *src = flow->dst_id, *dst = flow->src_id;

    return flow;
  }
}

/* ***************************************************** */
static struct ndpi_flow *get_ndpi_flow6(u_int16_t thread_id,
                    u_int16_t vlan_id,
                    const struct ndpi_ip6_hdr *iph6,
                    u_int16_t ip_offset,
                    struct ndpi_id_struct **src,
                    struct ndpi_id_struct **dst,
                    u_int8_t *proto) {
  struct ndpi_iphdr iph;

  memset(&iph, 0, sizeof(iph));
  iph.version = 4;
  iph.saddr = iph6->ip6_src.__u6_addr.__u6_addr32[2] + iph6->ip6_src.__u6_addr.__u6_addr32[3];
  iph.daddr = iph6->ip6_dst.__u6_addr.__u6_addr32[2] + iph6->ip6_dst.__u6_addr.__u6_addr32[3];
  iph.protocol = iph6->ip6_ctlun.ip6_un1.ip6_un1_nxt;

  if(iph.protocol == 0x3C /* IPv6 destination option */) {
    u_int8_t *options = (u_int8_t*)iph6 + sizeof(const struct ndpi_ip6_hdr);

    iph.protocol = options[0];
  }

  return(get_ndpi_flow(thread_id, 6, vlan_id, &iph, ip_offset,
               sizeof(struct ndpi_ip6_hdr),
               ntohs(iph6->ip6_ctlun.ip6_un1.ip6_un1_plen),
               src, dst, proto, iph6));
}

/* ***************************************************** */
static void updateFlowFeatures(u_int16_t thread_id,
                      struct ndpi_flow *flow,
                      const u_int64_t time,
                      u_int16_t vlan_id,
                      const struct ndpi_iphdr *iph,
                      struct ndpi_ip6_hdr *iph6,
                      u_int16_t ip_offset,
                      u_int16_t ipsize,
                      u_int16_t rawsize) {

    ndpi_thread_info[thread_id].stats.ip_packet_count++;
    ndpi_thread_info[thread_id].stats.total_wire_bytes += rawsize + 24 /* CRC etc */, ndpi_thread_info[thread_id].stats.total_ip_bytes += rawsize;
    if(flow->packets<500)
    {
        flow->inter_time[flow->packets] = time - flow->last_seen;
        flow->packet_size[flow->packets]=rawsize;
        flow->avg_packet_size  = (flow->avg_packet_size*flow->packets  + rawsize)/(flow->packets+1);
        flow->payload_avg_size = (flow->payload_avg_size*flow->packets + ipsize )/(flow->packets+1);
    }
    flow->packets++, flow->bytes += rawsize;
    flow->last_seen = time;

    if(flow->min_packet_size>rawsize)
        flow->min_packet_size = rawsize;

    if(flow->max_packet_size<rawsize)
        flow->max_packet_size = rawsize;

    if(flow->payload_min_size>ipsize)
        flow->payload_min_size = ipsize;

    if(flow->payload_max_size<ipsize)
        flow->payload_max_size = ipsize;

    flow->payload_bytes += ipsize;
    if(flow->packets==1)
    {
        flow->payload_first_size = ipsize;
        flow->first_seen=time;
    }

    if(ipsize==0)
        flow->packets_without_payload++;

    flow->flow_duration = time - flow->first_seen;
}
/* ***************************************************** */


// ipsize = header->len - ip_offset ; rawsize = header->len
static unsigned int packet_processing(u_int16_t thread_id,
				      const u_int64_t time,
				      u_int16_t vlan_id,
				      const struct ndpi_iphdr *iph,
				      struct ndpi_ip6_hdr *iph6,
				      u_int16_t ip_offset,
				      u_int16_t ipsize, u_int16_t rawsize) {
  struct ndpi_id_struct *src, *dst;
  struct ndpi_flow *flow;
  struct ndpi_flow_struct *ndpi_flow = NULL;
  u_int8_t proto;

  if(iph)
    flow = get_ndpi_flow(thread_id, 4, vlan_id, iph, ip_offset, ipsize,
			 ntohs(iph->tot_len) - (iph->ihl * 4),
			 &src, &dst, &proto, NULL);
  else
    flow = get_ndpi_flow6(thread_id, vlan_id, iph6, ip_offset, &src, &dst, &proto);

  if(flow != NULL) {
// static void updateFlowFeatures(u_int16_t *thread_id,
//                       struct ndpi_flow *flow, 
// 				      const u_int64_t *time,
// 				      u_int16_t *vlan_id,
// 				      const struct ndpi_iphdr *iph,
// 				      struct ndpi_ip6_hdr *iph6,
// 				      u_int16_t *ip_offset,
// 				      u_int16_t *ipsize, 
//                       u_int16_t *rawsize) {
     updateFlowFeatures(thread_id,flow,time,vlan_id,iph,iph6,ip_offset,ipsize,rawsize);
     ndpi_flow = flow->ndpi_flow;
  } else {
    return(0);
  }

  // TODO HZ
  // Interou 500 pacotes, salva no HBASE
  if(ndpi_thread_info[thread_id].stats.protocol_flows[flow->detected_protocol.protocol] == HOGZILLA_MAX_NDPI_FLOWS)
  {/*salva no HBASE*/return(0);}
  
  // TODO HZ
  // Conexão acabou? salva no HBASE e tira da árvore


  if(flow->detection_completed) return(0);

  flow->detected_protocol = ndpi_detection_process_packet(ndpi_thread_info[thread_id].ndpi_struct, ndpi_flow,
							  iph ? (uint8_t *)iph : (uint8_t *)iph6,
							  ipsize, time, src, dst);

  if((flow->detected_protocol.protocol != NDPI_PROTOCOL_UNKNOWN)
     || ((proto == IPPROTO_UDP) && (flow->packets > 8))
     || ((proto == IPPROTO_TCP) && (flow->packets > 10))) {
    flow->detection_completed = 1;

    if((flow->detected_protocol.protocol == NDPI_PROTOCOL_UNKNOWN) && (ndpi_flow->num_stun_udp_pkts > 0))
      ndpi_set_detected_protocol(ndpi_thread_info[thread_id].ndpi_struct, ndpi_flow, NDPI_PROTOCOL_STUN, NDPI_PROTOCOL_UNKNOWN);

    snprintf(flow->host_server_name, sizeof(flow->host_server_name), "%s", flow->ndpi_flow->host_server_name);

    if((proto == IPPROTO_TCP) && (flow->detected_protocol.protocol != NDPI_PROTOCOL_DNS)) {
      snprintf(flow->ssl.client_certificate, sizeof(flow->ssl.client_certificate), "%s", flow->ndpi_flow->protos.ssl.client_certificate);
      snprintf(flow->ssl.server_certificate, sizeof(flow->ssl.server_certificate), "%s", flow->ndpi_flow->protos.ssl.server_certificate);
    }

#if 0
    if(verbose > 1) {
      if(ndpi_is_proto(flow->detected_protocol, NDPI_PROTOCOL_HTTP)) {
	char *method;

	printf("[URL] %s\n", ndpi_get_http_url(ndpi_thread_info[thread_id].ndpi_struct, ndpi_flow));
	printf("[Content-Type] %s\n", ndpi_get_http_content_type(ndpi_thread_info[thread_id].ndpi_struct, ndpi_flow));

	switch(ndpi_get_http_method(ndpi_thread_info[thread_id].ndpi_struct, ndpi_flow)) {
	case HTTP_METHOD_OPTIONS: method = "HTTP_METHOD_OPTIONS"; break;
	case HTTP_METHOD_GET:     method = "HTTP_METHOD_GET"; break;
	case HTTP_METHOD_HEAD:    method = "HTTP_METHOD_HEAD"; break;
	case HTTP_METHOD_POST:    method = "HTTP_METHOD_POST"; break;
	case HTTP_METHOD_PUT:     method = "HTTP_METHOD_PUT"; break;
	case HTTP_METHOD_DELETE:  method = "HTTP_METHOD_DELETE"; break;
	case HTTP_METHOD_TRACE:   method = "HTTP_METHOD_TRACE"; break;
	case HTTP_METHOD_CONNECT: method = "HTTP_METHOD_CONNECT"; break;
	default:                  method = "HTTP_METHOD_UNKNOWN"; break;
	}

	printf("[Method] %s\n", method);
      }
    }
#endif

    free_ndpi_flow(flow);

    if(verbose > 1) {
      if(enable_protocol_guess) {
	if(flow->detected_protocol.protocol == NDPI_PROTOCOL_UNKNOWN) {
	  flow->detected_protocol.protocol = node_guess_undetected_protocol(thread_id, flow),
	    flow->detected_protocol.master_protocol = NDPI_PROTOCOL_UNKNOWN;
	}
      }

      printFlow(thread_id, flow);
    }
  }

#if 0
  if(ndpi_flow->l4.tcp.host_server_name[0] != '\0')
    printf("%s\n", ndpi_flow->l4.tcp.host_server_name);
#endif

  // TODO HZ:
  if(live_capture) {
    if(ndpi_thread_info[thread_id].last_idle_scan_time + IDLE_SCAN_PERIOD < ndpi_thread_info[thread_id].last_time) {
      /* scan for idle flows */
      ndpi_twalk(ndpi_thread_info[thread_id].ndpi_flows_root[ndpi_thread_info[thread_id].idle_scan_idx], node_idle_scan_walker, &thread_id);

      /* remove idle flows (unfortunately we cannot do this inline) */
      while (ndpi_thread_info[thread_id].num_idle_flows > 0)
	ndpi_tdelete(ndpi_thread_info[thread_id].idle_flows[--ndpi_thread_info[thread_id].num_idle_flows],
		     &ndpi_thread_info[thread_id].ndpi_flows_root[ndpi_thread_info[thread_id].idle_scan_idx], node_cmp);

      if(++ndpi_thread_info[thread_id].idle_scan_idx == NUM_ROOTS) ndpi_thread_info[thread_id].idle_scan_idx = 0;
      ndpi_thread_info[thread_id].last_idle_scan_time = ndpi_thread_info[thread_id].last_time;
    }
  }

  return 0;
}


void HogzillaSaveFlow(flow)
{
  // Salvar no HBASE
  //   . Ver se tá conectado, senao conecta
  //   . "insert" no banco
}

/*****
// CSP
// função existente no spo_log_tcpdump.c
// função HogzillaRollLogFile comentada no prototype mas definida aqui
// ver a necessidade de uso 
// PSC
void HogzillaReset(void)
{
    HogzillaRollLogFile(hogzilla_ptr);
}
*/

void DirectHogzilla(struct pcap_pkthdr *ph, uint8_t *pkt)
{
    size_t dumpSize = SizeOf(ph);

    if ( hogzilla_ptr->size + dumpSize > hogzilla_ptr->limit )
        TcpdumpRollLogFile(hogzilla_ptr);

    pc.log_pkts++;
    pcap_dump((u_char *)hogzilla_ptr->dumpd, ph, pkt);

    hogzilla_ptr->size += dumpSize;
}