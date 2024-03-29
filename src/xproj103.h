#define VMSTAT        0
#define SLABSTAT      0x00000004
#define BUFFERSIZE      8192
#define IFCPUSIZE	10
#define INTERVAL	(60*45)
#include <config.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <rpc/xdr.h>
#include <signal.h>
#include <sysexits.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/time.h>


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "proc/devname.h"
#include "proc/wchan.h"
#include "proc/procps.h"
#include "proc/readproc.h"
#include "proc/escape.h"
#include "proc/sig.h"
#include "proc/sysinfo.h"
#include "proc/version.h"
#include "proc/whattime.h"

#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include <syslog.h>
#include <pthread.h>

pthread_mutex_t buff_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t  buff_cond = PTHREAD_COND_INITIALIZER;

static unsigned long dataUnit=1024;


static int statMode=VMSTAT;
static unsigned sleep_time = 1;
static unsigned long num_updates = 8;

///////////////////////////////////////////////////////////////////////////

typedef struct struct_if {
	char *ip;
	char *name;
}struct_if;

////////////////////////////////////////////////////////////////////////////

typedef struct struct_cpu {
	unsigned long free_mem;
	unsigned long total_mem;
	jiff cpu_use;
}struct_cpu;
////////////////////////////////////////////////////////////////////////////

typedef struct struct_ifcpu {
	struct_cpu *cpu;
	struct_if **interface;
	char* buff_a_decoder;
}struct_ifcpu;

struct_ifcpu* ifcpu_rcv_array[IFCPUSIZE];
char buff[BUFFERSIZE];
int cltsck;

////////////////////////////////////////////////////////////////////////////
typedef struct struct_pth {
	char *hostorip;
	int port;
	
}struct_pth;

///////////////////////////////////////////////////////////////////////////

int argtoport(char* optarg) {
	int port = atoi(optarg);
	if ((port < 1) ||  (port > 65535) )
	{
		    perror("Erreur: Port hors plage.\n"); exit(EXIT_FAILURE);
	} else {	  
		    printf("Option --port active sur le port %d.\n", port);	  
	}
	return port;	
}

////////////////////////////////////////////////////////////////////////////

int isIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    struct sockaddr_in6 sb;
    int result = inet_pton(AF_INET , ipAddress, &(sa.sin_addr));
    if ((result == 0) && (ipAddress[0] != '\0')) 
    { 
      
      ipAddress = strtok(ipAddress, "%");

      result = inet_pton(AF_INET6 , ipAddress, &(sb.sin6_addr));
    }
    return result != 0;
}

////////////////////////////////////////////////////////////////////////////

__sighandler_t xp_sighandler(int num_sig, struct siginfo *info, void *vide)
{

  struct sigaction rien, ancien;
  printf("SIG: %d\n", num_sig);
  
  switch(num_sig) {
    
    case SIGSEGV:
      	printf("\t Catch Segmentation error...\n");

      break;
    case SIGTSTP:
      	
	printf("\tMise en pause...\n");
	// On rearme le gestionnaire de signaux sur le handler xp_sighandler
	rien.sa_handler = (__sighandler_t) xp_sighandler;
	rien.sa_flags = 0;
	sigemptyset(&rien.sa_mask);
	sigaction(SIGTSTP, &rien, &ancien);
	
	printf("\tPause.\n");
	kill(getpid(), SIGSTOP);
	printf("\tExecution...\n");
	
	sigaction(SIGTSTP, &ancien, NULL);
	printf("\tEn cours.\n");
    break;
    
    case SIGINT:
 
    //break;
    
    case SIGTERM:
	//s'occuper de la sortie standard afin d'éviter l'affichage du ^C
	
	printf("\tArrêt du serveur...\n");
	printf("\tFermeture du socket...\n");
	close(cltsck);
	exit(EXIT_SUCCESS);
    break;
    }
}

void free_ipaddress (struct_if** tofree)
{
  int i;
	for ( i=0; tofree[i] != NULL; i++ ) {
	  if (tofree[i]->ip != NULL){
	    free(tofree[i]->ip);
	    tofree[i]->ip = NULL;
	  }
	  if (tofree[i]->name != NULL) {
	    free(tofree[i]->name);
	    tofree[i]->name = NULL;	
	  }
	  free(tofree[i]);
	  tofree[i] = NULL;
	}
  free(tofree);
  tofree = NULL;
 
}


void syslog_ip_cpu(struct_cpu* cpu, struct_if* ip)
{
  struct timeval time;
  struct timezone zone;
  char format[] = "%s IP: %s Interface: %s, CPU Usage %d, Total Mem: %d, Free Mem: %d\n";

  char chartime[30];
  
  gettimeofday(&time,&zone);
  strftime(chartime, 30, "%d/%m/%Y %T ", localtime(&time.tv_sec));
  FILE* fd;
   if ((fd = (FILE*) fopen("/var/log/audit", "a")) < 0)
   {
      	  perror("cannot open or create /home/azman/audit");
	  exit(errno);
 
   }
  openlog("audit", LOG_PID, LOG_DAEMON);
  syslog(LOG_INFO, format,  chartime, ip->ip, ip->name, cpu->cpu_use, cpu->total_mem, cpu->free_mem);
  fprintf(fd, format, chartime,  ip->ip, ip->name, cpu->cpu_use, cpu->total_mem, cpu->free_mem);
  closelog();
  fclose(fd);
  
}

/* Code inspiré de l'exemple du man getifaddrs */
struct_if** ip_get(struct_if **ip_array)
{
    struct ifaddrs *ifaddr = NULL, *ifa = NULL;
    int family, s,i;
    char host[NI_MAXHOST];
    
    if ( ip_array == NULL) {
	  perror("ip_array memory allocation");
	  exit(EXIT_FAILURE);
    }
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    
    /* Walk through linked list, maintaining head pointer so we
     can free list later */
    i=0;
    for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        
        family = (int)ifa->ifa_addr->sa_family;
	
        /* For an AF_INET* interface address, display the address */
        
        if (family == AF_INET || family == AF_INET6) { // supression des address ipv6 || family == AF_INET6
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? (socklen_t)sizeof(struct sockaddr_in) :
                            (socklen_t)sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            
            // exclure le loopback des ip à récolter.
	    if (strcmp(ifa->ifa_name, "lo")) 
	    {
		if ( ip_array[i] == NULL) {
		    ip_array[i] = (struct_if*)calloc(1, sizeof(struct_if));
		}
		
		if ( ip_array[i] == NULL) {
		    perror("ip_array memory allocation");
		    exit(EXIT_FAILURE);
		}
		
		//printf("\ttaille: %d\n", strlen(host));
		if ( ip_array[i]->ip == NULL)
		    ip_array[i]->ip = calloc(1, strlen(host)+1);
		
		if ( ip_array[i]->ip == NULL) {
		    perror("ip_array->ip memory allocation");
		    exit(EXIT_FAILURE);
		}
		strncpy(ip_array[i]->ip,host,strlen(host)+1);
		
		if ( ip_array[i]->name == NULL) 
		    ip_array[i]->name = calloc(1, strlen(ifa->ifa_name)+1);
		if ( ip_array[i]->name == NULL) {
		    perror("ip_array->name memory allocation");
		    exit(EXIT_FAILURE);
		}
		
		strncpy(ip_array[i]->name,ifa->ifa_name,strlen(ifa->ifa_name)+1);
		//printf("\taddress: <%s> interface: %s\n address: %X\n", ip_array[i]->ip, ip_array[i]->name, (ip_array[i]));
		//printf("GET index: %d\n", i);
		i++;
	
	    }
        }
    }
    //printf("\taddress: <%s>\n interface: %s\n", ip_array[0]->ip, ip_array[0]->name);
    free(ifaddr);
 //   freeifaddrs(ifaddr);
    return ip_array;
}

////////////////////////////////////////////////////////////////////////////

static unsigned long unitConvert(unsigned int size){
    float cvSize;
    cvSize=(float)size/dataUnit*((statMode==SLABSTAT)?1:1024);
    return ((unsigned long) cvSize);
}


// Code inspiré de l'exemple du vmstat.c 
////////////////////////////////////////////////////////////////////////////

struct_cpu * cpu_get(struct_cpu *cpu_array) {
    const char format[]="%6lu %6lu %2u\n";
    unsigned int tog=0; /* toggle switch for cleaner code */
    unsigned int running,blocked,dummy_1,dummy_2;
    jiff cpu_use[2], cpu_nic[2], cpu_sys[2], cpu_idl[2], cpu_iow[2], cpu_xxx[2], cpu_yyy[2], cpu_zzz[2];
    jiff duse, dsys, didl, diow, dstl, Div, divo2,avg_cpu = 0;
    unsigned long pgpgin[2], pgpgout[2], pswpin[2], pswpout[2];
    unsigned int intr[2], ctxt[2];
    unsigned int sleep_half; 
    unsigned long i;
    int debt = 0;  // handle idle ticks running backwards
    long user_hz = sysconf(_SC_CLK_TCK);

    if ( cpu_array == NULL) {
      perror("cpu_array memory allocation");
      exit(EXIT_FAILURE);
    }
    
    sleep_half=(sleep_time/2);
    
    meminfo();
    
    getstat(cpu_use,cpu_nic,cpu_sys,cpu_idl,cpu_iow,cpu_xxx,cpu_yyy,cpu_zzz,
            pgpgin,pgpgout,pswpin,pswpout,
            intr,ctxt,
            &running,&blocked,
            &dummy_1, &dummy_2);
    
    duse= *cpu_use + *cpu_nic; 
    dsys= *cpu_sys + *cpu_xxx + *cpu_yyy;
    didl= *cpu_idl;
    diow= *cpu_iow;
    dstl= *cpu_zzz;
    Div= duse+dsys+didl+diow+dstl;
    divo2= Div/2UL;
   
    cpu_array->free_mem = kb_main_free;
    cpu_array->total_mem = kb_main_total;
    
    for(i=1;i<num_updates;i++) { /* \\\\\\\\\\\\\\\\\\\\ main loop ////////////////// */
        sleep(sleep_time);
        tog= !tog;
        
        meminfo();
        
        getstat(cpu_use+tog,cpu_nic+tog,cpu_sys+tog,cpu_idl+tog,cpu_iow+tog,cpu_xxx+tog,cpu_yyy+tog,cpu_zzz+tog,
                pgpgin+tog,pgpgout+tog,pswpin+tog,pswpout+tog,
                intr+tog,ctxt+tog,
                &running,&blocked,
                &dummy_1,&dummy_2);
        
        duse= cpu_use[tog]-cpu_use[!tog] + cpu_nic[tog]-cpu_nic[!tog];
        dsys= cpu_sys[tog]-cpu_sys[!tog] + cpu_xxx[tog]-cpu_xxx[!tog] + cpu_yyy[tog]-cpu_yyy[!tog];
        didl= cpu_idl[tog]-cpu_idl[!tog];
        diow= cpu_iow[tog]-cpu_iow[!tog];
        dstl= cpu_zzz[tog]-cpu_zzz[!tog];
        
        /* idle can run backwards for a moment -- kernel "feature" */
        if(debt){
            didl = (int)didl + debt;
            debt = 0;
        }
        if( (int)didl < 0 ){
            debt = (int)didl;
            didl = 0;
        }
        
        Div= duse+dsys+didl+diow+dstl;
        divo2= Div/2UL;
	
	avg_cpu += (user_hz*duse+divo2)/Div;
        printf(format,
               unitConvert(kb_main_free),unitConvert(kb_main_total),
               (unsigned)( (user_hz*duse+divo2)/Div ) /*us*/
               );
    }
  cpu_array->cpu_use = avg_cpu/num_updates;
  
  return cpu_array;
}

int xdr_if(XDR* xdrs, struct_if* ifstruc)
{
  long unsigned int lenip, lenname;
   if (xdrs->x_op == XDR_DECODE) // (xdrs->x_op == XDR_DECODE) ? ...
    {
       // if (ifstruc == NULL) ifstruc = calloc(1, sizeof(struct_if));
        if (ifstruc->name == NULL) {
        ifstruc->name = calloc(1, 1024);
	}
	if (ifstruc->ip == NULL) {
	ifstruc->ip = calloc(1, 1024);
	} 
	lenname = 1024;
	lenip = 1024;
      
    } else {
      
      lenname = strlen(ifstruc->name)+1lu;
      lenip = strlen(ifstruc->ip)+1lu;

    } 
   
   if ( xdr_string(xdrs, &ifstruc->name, (u_int)lenname) && xdr_string(xdrs, &ifstruc->ip, (u_int)lenip))
   {   
     return 1;
   } else
   {   
        //desalloue la memoire alloué à ip et name si le chemin 0 est empreinté, c à d qu'aucune données n'est récupéré dans le buffer_xdr.
     free(ifstruc->name);
     ifstruc->name = NULL;
     free(ifstruc->ip);
     ifstruc->ip = NULL;
     return 0;
   } 
    

}

bool xdr_cpu(XDR* xdrs, struct_cpu* cpustruc)
{
    if ( xdr_u_long(xdrs, (long unsigned int*)&cpustruc->cpu_use) && xdr_u_long(xdrs, &cpustruc->free_mem) && xdr_u_long(xdrs, &cpustruc->total_mem ))
      return 1;
    return 0;
}


////////////////////////////////////////////////////////////////////////////

bool Clt_snd(struct_if** if_array, struct_cpu* cpu_array, char* host, int port)
{
  int sock, rcved,i;
  XDR xdrs;
  char buf[BUFFERSIZE];
  struct hostent *hostinfo =  { 0 };
  hostinfo = gethostbyname(host);
  
  memset(buf, 0, BUFFERSIZE);
  // Si l'initialisation de la structure hostinfo échoue.
  if (hostinfo == NULL)
  {
	perror("gethostbyname()");
	exit(h_errno);
  }
  
  //Si la création du socket échoue.
  struct sockaddr_in sin = { 0 }; 
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == -1)
  {
	perror("socket()");
	exit(errno);
  } 
  
  
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port); // htons converti le port
  memcpy(&sin.sin_addr.s_addr, hostinfo->h_addr, hostinfo->h_length);
  
  // Si la connection échoue.
  if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) 
  {
    //free(cpu_array);
    //free_ipaddress(if_array);
   
    perror("connect() next attempt in 45 min");
    return false;
  }
 
  // Crée un pointeur xdrs en mémoire et l'associe à buf, pour l'encodage
  xdrmem_create(&xdrs, buf, BUFFERSIZE, XDR_ENCODE);
  
  // Ajoute la structure cpu_array au buf attaché au pointeur xdrs.
  xdr_cpu(&xdrs, cpu_array);

  for ( i=0; if_array[i] != NULL; i++ )
  {
	      printf("ip: %s index: %d\n", if_array[i]->ip, i);
	      xdr_if(&xdrs, if_array[i]);
  }

	if(send(sock,buf, BUFFERSIZE, 0) != BUFFERSIZE)  {
		
	    perror("send()");
	    close(sock);
	    exit(errno);
		
	}
  
  close(sock);
  return false;
}

////////////////////////////////////////////////////////////////////////////

void parser_xdr(void* rien)
{
  
	  int x, y = 0;
	  XDR xdrs;
	  struct_if** if_rcv_array;	  
    for (;;) {
	  for (x = 0;x < IFCPUSIZE;x++) {
	    
	      pthread_mutex_lock(&buff_mutex);
	      printf("Mutex locker par parser\n");
	      	      
	      // Creation d'un "parser" XDR afin de reconstruire les données reçues dans buff
	      xdrmem_create(&xdrs, ifcpu_rcv_array[x]->buff_a_decoder, BUFFERSIZE, XDR_DECODE);
	     
	      xdr_cpu(&xdrs, ifcpu_rcv_array[x]->cpu);
			
	      printf("CPU: %lu\n", (long unsigned)ifcpu_rcv_array[x]->cpu->cpu_use);
	      printf("FREE MEM: %lu\n", ifcpu_rcv_array[x]->cpu->free_mem);
	      printf("FREE TOT: %lu\n", ifcpu_rcv_array[x]->cpu->total_mem);
	      
	      while (xdr_if(&xdrs, ifcpu_rcv_array[x]->interface[y]))
	      { 
		  if (!isIpAddress(ifcpu_rcv_array[x]->interface[y]->ip)) break;
		  printf("Interface: %s\n", ifcpu_rcv_array[x]->interface[y]->name);
		  printf("IP: <%s>\n", ifcpu_rcv_array[x]->interface[y]->ip);
		  
		  syslog_ip_cpu(ifcpu_rcv_array[x]->cpu, ifcpu_rcv_array[x]->interface[y]);
		  y++;
		  if( ifcpu_rcv_array[x]->interface[y] == NULL)  {
		    
		  ifcpu_rcv_array[x]->interface[y] = calloc(1, sizeof(struct_if));
		  
		  }
	      }
	      y = 0;
	      
	      xdr_destroy(&xdrs);
	      //pthread_cond_wait(&buff_cond, &buff_mutex);
	      pthread_mutex_unlock(&buff_mutex);
	      printf("Mutex UNlocker par parser, INDEX: %d\n", x);

	      pthread_mutex_lock(&cond_mutex);
	     
	      pthread_cond_wait(&buff_cond, &cond_mutex);

	      pthread_mutex_unlock(&cond_mutex);
	    
	  }
    }
}

////////////////////////////////////////////////////////////////////////////

void alloc_ifcpu(void* rien)
{
	int a, b;
	for (a = 0; a < IFCPUSIZE ;a++) {	  

		  ifcpu_rcv_array[a] = (struct_ifcpu*) malloc(sizeof(struct_ifcpu));		
		  ifcpu_rcv_array[a]->cpu = (struct_cpu*) malloc(sizeof(struct_cpu));				//   printf("0x%X\n", cpu_rcv_array[i]);			  
		  ifcpu_rcv_array[a]->interface = (struct_if**)malloc(sizeof(struct_if**));;
		  // Allocation de la premiere structure struct_if (On sait qu'il y aura au moins une interface)
		  ifcpu_rcv_array[a]->interface[0] = calloc(1, sizeof(struct_if));
		  ifcpu_rcv_array[a]->buff_a_decoder = calloc(1, BUFFERSIZE*sizeof(char));

	}
}

////////////////////////////////////////////////////////////////////////////

bool srv_rcv(void* ipport)
{
  struct_pth* arg = (struct_pth*)ipport;
  int port = arg->port;
  char* host = arg->hostorip;
  int sock, rcved,i, j;

  pthread_t th2 = 0, th_alloc;
  void* ret;
    
  struct sockaddr_in srv_sin = { 0 }, clt_sin =  { 0 }; 
  sock = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);

  if (sock == -1)
  {
	perror("socket()");
	exit(errno);
  } 
  
  srv_sin.sin_family = AF_INET;
  srv_sin.sin_port = htons(port);
  srv_sin.sin_addr.s_addr = htonl(INADDR_ANY);
  


  
  if (bind(sock, (struct sockaddr *) &srv_sin, sizeof(srv_sin)) < 0) 
  {
    perror("bind()");
    exit(errno);
  }
  
  if (listen(sock, 5) < 0) 
  {
    perror("listen()");
    exit(errno);
  }
  
  if (pthread_create(&th_alloc, NULL, alloc_ifcpu, NULL) <0 ) {
		    fprintf(stderr, "thread allocation erreur, \n");
  }
  i = 0;
  /* Run until cancelled */
  for (;;) {
           unsigned int clientlen = sizeof(clt_sin);

           /* Wait for client connection */
           if ((cltsck = accept(sock, (struct sockaddr *)&clt_sin, &clientlen)) < 0) 
	   {
                 perror("accept()");
		 exit(errno);
           }
           fprintf(stdout, "Client connected: %s\n", inet_ntoa(clt_sin.sin_addr));
           
	   if (( rcved = recv( cltsck, buff, BUFFERSIZE, 0)) < 0)
	   {
		 perror("recv()");
		 exit(errno);
	   }
	   
	   
	   pthread_mutex_lock(&buff_mutex);
	   printf("Mutex locker par serveur: INDEX: %d\n", i);
	   
	   memcpy(ifcpu_rcv_array[i]->buff_a_decoder, buff, BUFFERSIZE);
		   

	   pthread_mutex_unlock(&buff_mutex);
	   printf("Mutex UNlocker par serveur\n");
	   
	   pthread_mutex_lock(&cond_mutex);
	   
	   pthread_cond_signal(&buff_cond);

	   pthread_mutex_unlock(&cond_mutex);

	   
	   if (th2 == 0) {
	      if (pthread_create(&th2, NULL, parser_xdr, NULL) <0 ) {
		    fprintf(stderr, "thread parser XDR erreur, \n");
	      }
	   }
	
	      
	      close(cltsck);
	     

	   	i++;
	   if (i >= IFCPUSIZE) {
	     	 printf("REINIT INDEX: %d\n", i);

		i = 0;
	  }
	
		
	   
  }
  
  // portion de code n'est jamais atteinte car les seules sorties son les exit(errno) en cas d'erreur et le SIGINT.
  // Dans les deux cas, la fin du programme ce fera, aucun besoin de gérer les fuites car le noyau reclamera toute la memoire allouée au processus.
  // on efface la dernière allocation nécesseraiement vide car if_rcv_array[j] est préallouer à la j-1 eme itération avant la sortie de bloucle. 	 

//  free(cpu_rcv_array);
//  free_ipaddress(if_rcv_array);          
 
  return false;
}


