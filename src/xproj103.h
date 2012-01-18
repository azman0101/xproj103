#define VMSTAT        0
#define SLABSTAT      0x00000004
#define BUFFSIZE      (64*1024) 
#include <string.h>
#include <sys/socket.h>

static unsigned long dataUnit=1024;


//#define FALSE 0
//#define TRUE 1

static int statMode=VMSTAT;
static unsigned sleep_time = 1;
static unsigned long num_updates = 8;

///////////////////////////////////////////////////////////////////////////
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

/*
char *safe_strdup (const char *s)
{
	char *p;
 	size_t l;
    
 	if (!s || !*s) return 0;
 	l = strlen (s) + 1;
 	p = (char *)malloc (l);
 	memcpy (p, s, l);
 	return (p);
}
*/

void free_ipaddress (struct_if** tofree)
{
  int i;
	for ( i=0; tofree[i] != NULL; i++ ) {
	  if (tofree[i]->ip != NULL)
	    free(tofree[i]->ip);
	  if (tofree[i]->name != NULL)
	    free(tofree[i]->name);
	  free(tofree[i]);

	}
}


/* Code inspiré de l'exemple du man getifaddrs */
struct_if** ip_get(struct_if **ip_array)
{
    struct ifaddrs *ifaddr, *ifa;
    int family, s,i;
    char host[NI_MAXHOST];
    if (ip_array == NULL)
      ip_array = malloc(sizeof(struct_if));
    
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
        
        family = ifa->ifa_addr->sa_family;
	
        /* For an AF_INET* interface address, display the address */
        
        if (family == AF_INET || family == AF_INET6) { // supression des address ipv6 || family == AF_INET6
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            
            // exclure le loopback des ip à récolter.
	    if (strcmp(ifa->ifa_name, "lo")) 
	    {
	    
		ip_array[i] = calloc(1, sizeof(struct_if));
		
		if ( ip_array[i] == NULL) {
		    perror("ip_array memory allocation");
		    exit(EXIT_FAILURE);
		}
		
		//printf("\ttaille: %d\n", strlen(host));
		ip_array[i]->ip =(char*)malloc(strlen(host)+1);
		if ( ip_array[i]->ip == NULL) {
		    perror("ip_array->ip memory allocation");
		    exit(EXIT_FAILURE);
		}
		strncpy(ip_array[i]->ip,host,strlen(host)+1);
		
		ip_array[i]->name = (char*)malloc(strlen(ifa->ifa_name)+1);
		if ( ip_array[i]->name == NULL) {
		    perror("ip_array->name memory allocation");
		    exit(EXIT_FAILURE);
		}
		
		strncpy(ip_array[i]->name,ifa->ifa_name,strlen(ifa->ifa_name)+1);
		i++;
	    }
        }
    }
    //printf("\taddress: <%s>\n interface: %s\n", ip_array[0]->ip, ip_array[0]->name);
    
    freeifaddrs(ifaddr);
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
    unsigned long kb_per_page = sysconf(_SC_PAGESIZE) / 1024ul;
    int i, debt = 0;  // handle idle ticks running backwards
    long user_hz = sysconf(_SC_CLK_TCK);

    if (cpu_array == NULL)
      cpu_array = malloc(sizeof(struct_cpu));
    
    
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
   
            
    if ( cpu_array == NULL) {
      perror("cpu_array memory allocation");
      exit(EXIT_FAILURE);
    }
	    
   
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
	
	avg_cpu += (100*duse+divo2)/Div;
        printf(format,
               unitConvert(kb_main_free),unitConvert(kb_main_total),
               (unsigned)( (100*duse+divo2)/Div ) /*us*/
               );
    }
  cpu_array->cpu_use = avg_cpu/num_updates;
  
  return cpu_array;
}

bool_t xdr_if(XDR* xdrs, struct_if* ifstruc)
{
  int lenip, lenname;
   if (xdrs->x_op == XDR_DECODE) // (xdrs->x_op == XDR_DECODE) ? ...
    {
        ifstruc->name = (char*)malloc(1024);
	ifstruc->ip = (char*)malloc(1024);
	lenname = 1024;
	lenip = 1024;
      
    } else {
      
      lenname = strlen(ifstruc->name)+1;
      lenip = strlen(ifstruc->ip)+1;

    } 
   
   if ( xdr_string(xdrs, &ifstruc->name, lenname) && xdr_string(xdrs, &ifstruc->ip, lenip))
   {   
     return 1;
   } else
   {   
     free(ifstruc->name);
     free(ifstruc->ip);
     return 0;
   } 
    
   //desallouer la memoire alloué à ip et name si le chemin 0 est empreinté.
}

bool_t xdr_cpu(XDR* xdrs, struct_cpu* cpustruc)
{
    if ( xdr_u_long(xdrs, (long unsigned int*)&cpustruc->cpu_use) && xdr_u_long(xdrs, &cpustruc->free_mem) && xdr_u_long(xdrs, &cpustruc->total_mem ))
      return 1;
    return 0;
}


////////////////////////////////////////////////////////////////////////////

int Clt_snd(struct_if** if_array, struct_cpu* cpu_array, char* host, int port)
{
  int sock, rcved, rcvlen,i;
  FILE *t;
  XDR xdrs;
  char buf[8192];
  struct hostent *hostinfo = gethostbyname(host);
  
  // Si l'initialisation de la structure hostinfo échoue.
  if (hostinfo == NULL)
  {
	perror("gethostbyname()");
	exit(h_errno);
  }
  
  //Si la création du socket échoue.
  struct sockaddr_in sin = { 0 }; 
  sock = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
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
    perror("connect()");
    exit(errno);
  }
  u_int size = NI_MAXHOST; //TODO: résoudre le probleme de buff fixe.
 // buf = (char*)malloc(size);
  
  // Crée un pointeur xdrs en mémoire et l'associe à buf, pour l'encodage
  xdrmem_create(&xdrs, buf, 4096, XDR_ENCODE);
  
  // Ajoute la structure cpu_array au buf attaché au pointeur xdrs.
  xdr_cpu(&xdrs, cpu_array);
  
  for ( i=0; if_array[i] != NULL; i++ )
   {
	//printf("ip: %s index: %d\n", if_array[i]->ip, i);
	xdr_if(&xdrs, if_array[0]);
   }
      
   
  for ( i=0; if_array[i] != NULL; i++ ) {

      rcvlen = sizeof(buf);
      if(send(sock,buf, sizeof(buf), 0) != rcvlen)  {
	perror("send()");
	exit(errno);
      }
  
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////

int srv_rcv( char* host, int port)
{
  
  int sock, cltsck, rcved,i, j;
  char buff[8192];
  XDR xdrs;
  struct_if** if_rcv_array;
  struct_cpu** cpu_rcv_array;
  struct sockaddr_in srv_sin = { 0 }, clt_sin =  { 0 }; 
  sock = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);
  if (sock == -1)
  {
	perror("socket()");
	exit(errno);
  } 
  
  srv_sin.sin_family = AF_INET;
  srv_sin.sin_port = htons(port);
  if ( host == NULL)
    srv_sin.sin_addr.s_addr = htonl(INADDR_ANY);
  else
    srv_sin.sin_addr.s_addr = htonl(INADDR_ANY); // TODO: remplacer par host mais en utilisant la bonne fonction


  
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
     
   cpu_rcv_array = malloc(sizeof(struct_cpu));
   
   if ( cpu_rcv_array == NULL) {
      perror("cpu_rcv_array memory allocation");
      exit(EXIT_FAILURE);
   }
    
   if_rcv_array = malloc(sizeof(struct_if));
   
   if ( if_rcv_array == NULL) {
      perror("if_rcv_array memory allocation");
      exit(EXIT_FAILURE);
   }
 	/* Run until cancelled */
	

  for ( i = 0;;i++) {
           unsigned int clientlen = sizeof(clt_sin);
	   cpu_rcv_array[i] = calloc(1, sizeof(struct_cpu));
	   if_rcv_array[j] = calloc(1, sizeof(struct_if));
	   	  
           /* Wait for client connection */
           if ((cltsck = accept(sock, (struct sockaddr *)&clt_sin, &clientlen)) < 0) 
	   {
                 perror("accept()");
		 exit(errno);
           }
           fprintf(stdout, "Client connected: %s\n", inet_ntoa(clt_sin.sin_addr));
          
	   if (( rcved = recv( cltsck, buff, 8192, 0)) < 0)
	   {
		 perror("recv()");
		 exit(errno);
	   }
	   
	     xdrmem_create(&xdrs, buff, sizeof(buff), XDR_DECODE);
	     
	     xdr_cpu(&xdrs, cpu_rcv_array[i]);

	     printf("CPU: %lu\n", (long unsigned)cpu_rcv_array[i]->cpu_use);
	     printf("FREE MEM: %lu\n", cpu_rcv_array[i]->free_mem);
	     printf("FREE TOT: %lu\n", cpu_rcv_array[i]->total_mem);
	     
	     	
	     for (j = 0; xdr_if(&xdrs, if_rcv_array[j]);)
	     { 
	       printf("Interface: %s\n", if_rcv_array[j]->name);
	       printf("IP: <%s>\n", if_rcv_array[j]->ip);
	       j++;
	       if_rcv_array[j] = calloc(1, sizeof(struct_if));
	      }


	     
	    close(cltsck);
	   
  }
  free(cpu_rcv_array);
  free(if_rcv_array);          
 
  return 0;
}
