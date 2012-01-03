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
static unsigned long num_updates;

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

typedef struct data_struct {
	char ip[16];
	char name[6];
	unsigned long free_mem;
	unsigned long total_mem;  // index into a struct disk_stat array
	unsigned long long cpu_use;
}data_struct;

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

/* Code inspiré de la fonction de WineHQ AllocateAndGetIpNetTableFromStack */
char * ip_get(char* if_name)
{
    FILE *fp;
    char ip_addr[16];
   // char if_name[6];
    char * reply = NULL;

    if ((fp = fopen("/proc/net/arp", "r")))
    {
        char buf[512], *ptr;
        unsigned long flags;
        
        /* skip header line */
        endofline(fp, '\n');
        ptr = fgets(buf, sizeof(buf), fp);
        while ((ptr = fgets(buf, sizeof(buf), fp)))
        {
            strncpy((char*)ip_addr,ptr,sizeof(ip_addr));
            //ip_addr = ptr;
            while (*ptr && !isspace(*ptr)) ptr++;
            strtoul(ptr + 1, &ptr, 16); /* hw type (skip) */
            flags = strtoul(ptr + 1, &ptr, 16);
            
            while (*ptr && isspace(*ptr)) ptr++;
            while (*ptr && !isspace(*ptr))
            {
                strtoul(ptr, &ptr, 16);
                if (*ptr) ptr++;
            }
            while (*ptr && isspace(*ptr)) ptr++;
            while (*ptr && !isspace(*ptr)) ptr++;   /* mask (skip) */
            while (*ptr && isspace(*ptr)) ptr++;
            strncpy(if_name,ptr,6);
            
         //   if (!(table = append_ipnet_row( heap, flags, table, &count, &row )))
            //    break;
        }
        fclose(fp);
    }
    else return NULL;


       return reply;
}

char* arp_get(data_struct **ip_array)
{
  FILE *proc;
  char if_ip[16];
  char if_name[6];
  char * reply = NULL;
  int i = 0;
  
  if (!(proc = fopen("/home/azman/arp", "r"))) {
    return NULL;
  }

  fscanf(proc, "%*s %*s %*s %*s %*s %*s %*s %*s %*s");
  
  while(!feof(proc)) {
    fscanf(proc, "%15s %*s %*s %*s %*s %6s", if_ip, if_name);
    // tester si ip est une ip et
    ip_array[i] = (data_struct*)malloc(sizeof(data_struct));
    ip_array[i]->ip = (char[16])if_ip;
    ip_array[i]->name = (char[6])if_name;
    
  }
 fclose(proc);

 return NULL;
}



static unsigned long unitConvert(unsigned int size){
 float cvSize;
 cvSize=(float)size/dataUnit*((statMode==SLABSTAT)?1:1024);
 return ((unsigned long) cvSize);
}

////////////////////////////////////////////////////////////////////////////

static void new_header(void){
  printf("procs -----------memory---------- ---swap-- -----io---- -system-- ----cpu----\n");
  printf(
    "%2s %2s %6s %6s %6s %6s %6s %4s %4s %5s %5s %4s %4s %2s %2s %2s %2s\n",
    "r","b",
    "swpd", "free", "tot", a_option?"inact":"buff", a_option?"active":"cache",
    "si","so",
    "bi","bo",
    "in","cs",
    "us","sy","id","wa"
  );
}

////////////////////////////////////////////////////////////////////////////

static void new_format(void) {
  const char format[]="%2u %2u %6lu %6lu %6lu %6lu %6lu %4u %4u %5u %5u %4u %4u %2u %2u %2u %2u\n";
  unsigned int tog=0; /* toggle switch for cleaner code */
  unsigned int i;
  unsigned int hz = Hertz;
  unsigned int running,blocked,dummy_1,dummy_2;
  jiff cpu_use[2], cpu_nic[2], cpu_sys[2], cpu_idl[2], cpu_iow[2], cpu_xxx[2], cpu_yyy[2], cpu_zzz[2];
  jiff duse, dsys, didl, diow, dstl, Div, divo2;
  unsigned long pgpgin[2], pgpgout[2], pswpin[2], pswpout[2];
  unsigned int intr[2], ctxt[2];
  unsigned int sleep_half; 
  unsigned long kb_per_page = sysconf(_SC_PAGESIZE) / 1024ul;
  int debt = 0;  // handle idle ticks running backwards
  data_struct monitoring;
  sleep_half=(sleep_time/2);
  new_header();
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
  printf(format,
	 running, blocked,
	 unitConvert(kb_swap_used), unitConvert(kb_main_free),unitConvert(kb_main_total),
	 unitConvert(a_option?kb_inactive:kb_main_buffers),
	 unitConvert(a_option?kb_active:kb_main_cached),
	 (unsigned)( (*pswpin  * unitConvert(kb_per_page) * hz + divo2) / Div ),
	 (unsigned)( (*pswpout * unitConvert(kb_per_page) * hz + divo2) / Div ),
	 (unsigned)( (*pgpgin                * hz + divo2) / Div ),
	 (unsigned)( (*pgpgout               * hz + divo2) / Div ),
	 (unsigned)( (*intr                  * hz + divo2) / Div ),
	 (unsigned)( (*ctxt                  * hz + divo2) / Div ),
	 (unsigned)( (100*duse                    + divo2) / Div ),
	 (unsigned)( (100*dsys                    + divo2) / Div ),
	 (unsigned)( (100*didl                    + divo2) / Div ),
	 (unsigned)( (100*diow                    + divo2) / Div ) /* ,
	 (unsigned)( (100*dstl                    + divo2) / Div ) */
  );
  monitoring.cpu_use = duse;

  for(i=1;i<num_updates;i++) { /* \\\\\\\\\\\\\\\\\\\\ main loop ////////////////// */
    sleep(sleep_time);
    if (moreheaders && ((i%height)==0)) new_header();
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
}

////////////////////////////////////////////////////////////////////////////


static void sum_format(void) {
  unsigned int running, blocked, btime, processes;
  jiff cpu_use, cpu_nic, cpu_sys, cpu_idl, cpu_iow, cpu_xxx, cpu_yyy, cpu_zzz;
  unsigned long pgpgin, pgpgout, pswpin, pswpout;
  unsigned int intr, ctxt;

  meminfo();

  getstat(&cpu_use, &cpu_nic, &cpu_sys, &cpu_idl,
          &cpu_iow, &cpu_xxx, &cpu_yyy, &cpu_zzz,
	  &pgpgin, &pgpgout, &pswpin, &pswpout,
	  &intr, &ctxt,
	  &running, &blocked,
	  &btime, &processes);

  printf("%13lu %s total memory\n", unitConvert(kb_main_total),szDataUnit);
  printf("%13lu %s used memory\n", unitConvert(kb_main_used),szDataUnit);
  printf("%13lu %s active memory\n", unitConvert(kb_active),szDataUnit);
  printf("%13lu %s inactive memory\n", unitConvert(kb_inactive),szDataUnit);
  printf("%13lu %s free memory\n", unitConvert(kb_main_free),szDataUnit);
  printf("%13lu %s buffer memory\n", unitConvert(kb_main_buffers),szDataUnit);
  printf("%13lu %s swap cache\n", unitConvert(kb_main_cached),szDataUnit);
  printf("%13Lu non-nice user cpu ticks\n", cpu_use);
  printf("%13Lu nice user cpu ticks\n", cpu_nic);
  printf("%13Lu system cpu ticks\n", cpu_sys);
  printf("%13Lu idle cpu ticks\n", cpu_idl);
  printf("%13Lu IO-wait cpu ticks\n", cpu_iow);
  printf("%13Lu IRQ cpu ticks\n", cpu_xxx);
  printf("%13Lu softirq cpu ticks\n", cpu_yyy);
  printf("%13Lu stolen cpu ticks\n", cpu_zzz);
  printf("%13lu pages paged in\n", pgpgin);
  printf("%13lu pages paged out\n", pgpgout);
  printf("%13lu pages swapped in\n", pswpin);
  printf("%13lu pages swapped out\n", pswpout);
  printf("%13u interrupts\n", intr);
  printf("%13u CPU context switches\n", ctxt);
  printf("%13u boot time\n", btime);
  printf("%13u forks\n", processes);
}

////////////////////////////////////////////////////////////////////////////
