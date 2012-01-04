#define VMSTAT        0
#define SLABSTAT      0x00000004
#define BUFFSIZE      (64*1024) 

static unsigned long dataUnit=1024;
static char szDataUnit [16];


#define FALSE 0
#define TRUE 1

static int a_option; /* "-a" means "show active/inactive" */
static unsigned int height;   // window height
static unsigned int moreheaders=TRUE;

static int a_option; /* "-a" means "show active/inactive" */
static int statMode=VMSTAT;
static unsigned sleep_time = 1;
static unsigned long num_updates = 10;

/////////////////////////////////////////////////////////////////////////////

static int endofline(FILE *ifp, int c)
{
    int eol = (c == '\r' || c == '\n');
    if (c == '\r')
    {
        c = getc(ifp);
        if (c != '\n' && c != EOF)
            ungetc(c, ifp);
    }
    return(eol);
}

///////////////////////////////////////////////////////////////////////////

typedef struct struct_if {
	char ip[NI_MAXHOST];
	char *name;
}struct_if;

////////////////////////////////////////////////////////////////////////////


typedef struct struct_cpu {
	unsigned long free_mem;
	unsigned long total_mem;  // index into a struct disk_stat array
	jiff cpu_use;
}struct_cpu;

////////////////////////////////////////////////////////////////////////////

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
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        
        family = ifa->ifa_addr->sa_family;
        
        /* Display interface name and family (including symbolic
         form of the latter for the common families) */
        
        printf("%s  address family: %d%s\n",
               ifa->ifa_name, family,
               (family == AF_PACKET) ? " (AF_PACKET)" :
               (family == AF_INET) ?   " (AF_INET)" :
               (family == AF_INET6) ?  " (AF_INET6)" : "");
        
        /* For an AF_INET* interface address, display the address */
        
        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\taddress: <%s>\n", host);
            ip_array[i] = calloc(1, sizeof(struct_if));
            
            if ( ip_array[i] == NULL) {
                perror("ip_array memory allocation");
                exit(EXIT_FAILURE);
            }
	    
            strncpy(ip_array[i]->ip,host,sizeof(host));
	    ip_array[i]->name = (char*)malloc(sizeof(ifa->ifa_name)+1);
	    strncpy(ip_array[i]->name,ifa->ifa_name,sizeof(ifa->ifa_name));
           // ip_array[i]->name = ifa->ifa_name;
	    i++;
        }
    }
    //printf("\taddress: <%s>\n interface: %s\n", ip_array[0]->ip, ip_array[0]->name);
    freeifaddrs(ifaddr);
    return ip_array;
}

/*
char* arp_get(data_struct **ip_array)
{
    FILE *proc;
    char* if_ip;
    char* if_name;
    char* reply = NULL;
    int i = 0;
    i++;
    if (!(proc = fopen("/home/azman/arp", "r"))) {
        return NULL;
    }
    
    fscanf(proc, "%*s %*s %*s %*s %*s %*s %*s %*s %*s");
    
    while(!feof(proc)) {
        fscanf(proc, "%15s %*s %*s %*s %*s %6s", if_ip, if_name);
        // tester si ip est une ip et
        ip_array[i] = (data_struct*)malloc(sizeof(data_struct));
      //  ip_array[i]->ip = if_ip;
      //  ip_array[i]->name = if_name;
        
    }
    fclose(proc);
    
    return NULL;
}
*/


static unsigned long unitConvert(unsigned int size){
    float cvSize;
    cvSize=(float)size/dataUnit*((statMode==SLABSTAT)?1:1024);
    return ((unsigned long) cvSize);
}



////////////////////////////////////////////////////////////////////////////

struct_cpu * new_format(struct_cpu *cpu_array) {
    const char format[]="%2u %2u %6lu %6lu %6lu %6lu %6lu %4u %4u %5u %5u %4u %4u %2u %2u %2u %2u\n";
    unsigned int tog=0; /* toggle switch for cleaner code */
    unsigned int hz = Hertz;
    unsigned int running,blocked,dummy_1,dummy_2;
    jiff cpu_use[2], cpu_nic[2], cpu_sys[2], cpu_idl[2], cpu_iow[2], cpu_xxx[2], cpu_yyy[2], cpu_zzz[2];
    jiff duse, dsys, didl, diow, dstl, Div, divo2,avg_cpu = 0;
    unsigned long pgpgin[2], pgpgout[2], pswpin[2], pswpout[2];
    unsigned int intr[2], ctxt[2];
    unsigned int sleep_half; 
    unsigned long kb_per_page = sysconf(_SC_PAGESIZE) / 1024ul;
    int i, debt = 0;  // handle idle ticks running backwards
    int user_hz = sysconf(_SC_CLK_TCK);
    
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
        //if (moreheaders && ((i%height)==0)) new_header();
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
               running, blocked,
               unitConvert(kb_swap_used),unitConvert(kb_main_free),unitConvert(kb_main_total),
               unitConvert(a_option?kb_inactive:kb_main_buffers),
               unitConvert(a_option?kb_active:kb_main_cached),
               (unsigned)( ( (pswpin [tog] - pswpin [!tog])*unitConvert(kb_per_page)+sleep_half )/sleep_time ), /*si*/
               (unsigned)( ( (pswpout[tog] - pswpout[!tog])*unitConvert(kb_per_page)+sleep_half )/sleep_time ), /*so*/
               (unsigned)( (  pgpgin [tog] - pgpgin [!tog]             +sleep_half )/sleep_time ), /*bi*/
               (unsigned)( (  pgpgout[tog] - pgpgout[!tog]             +sleep_half )/sleep_time ), /*bo*/
               (unsigned)( (  intr   [tog] - intr   [!tog]             +sleep_half )/sleep_time ), /*in*/
               (unsigned)( (  ctxt   [tog] - ctxt   [!tog]             +sleep_half )/sleep_time ), /*cs*/
               (unsigned)( (100*duse+divo2)/Div ), /*us*/
               (unsigned)( (100*dsys+divo2)/Div ), /*sy*/
               (unsigned)( (100*didl+divo2)/Div ), /*id*/
               (unsigned)( (100*diow+divo2)/Div )/*, //wa
                                                  (unsigned)( (100*dstl+divo2)/Div )  //st  */
               );
    }
  cpu_array->cpu_use = avg_cpu/num_updates;

  return cpu_array;
}

////////////////////////////////////////////////////////////////////////////
