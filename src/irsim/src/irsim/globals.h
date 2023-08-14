
	/* EXPORTS FROM access.c */

typedef	struct
  {
    char  exist;
    char  read;
    char  write;
  } Fstat;
extern Fstat *FileStatus( /*  name */ );

	/* EXPORTS FROM binsim.c */

extern void wr_netfile( /*  fname */ );
extern int rd_netfile( /*  f, line */ );
extern void bin_connect_txtors( /* */ );

	/* EXPORTS FROM cad_dir.c */

extern char    *cad_lib;
extern char    *cad_bin;
extern void InitCAD( /* */ );

	/* EXPORTS FROM config.c */

extern double  CM2A ;
extern double  CM2P ;
extern double  CMA ;
extern double  CMP ;
extern double  CPA ;
extern double  CPP ;
extern double  CDA ;
extern double  CDP ;
extern double  CPDA ;
extern double  CPDP ;
extern double  CGA ;
extern double  LAMBDA ;
extern double  LAMBDA2;
extern double  LOWTHRESH ;
extern double  HIGHTHRESH ;
extern double  DIFFEXT ;
extern int	config_flags ;
#define	DIFFPERIM	0x1	/* set if diffusion perimeter does not 	    */
#define	CNTPULLUP	0x2	/* set if capacitance from gate of pullup   */
#define	SUBPAREA	0x4	/* set if poly over xistor doesn't make a   */
#define	DIFFEXTF	0x8	/* set if we should add capacitance due to  */
extern void config( /*  cname */ );
extern void requiv( /*  width, length, type, rp */ );

	/* EXPORTS FROM conn_list.c */

#define	MAX_PARALLEL	30	/* this is probably sufficient per stage */
extern tptr  parallel_xtors[ /* MAX_PARALLEL */ ];
#define	par_list( T )		( parallel_xtors[ (T)->n_par ] )
extern void BuildConnList( /*  n */ );
extern void WarnTooManyParallel( /* */ );

	/* EXPORTS FROM eval.c */

#define	NMODEL		2		/* number of models supported */
extern ifun   new_value[ /*NMODEL*/ ] ;
extern int    model ;
extern int    sm_stat ;
extern char   vchars[ /**/ ] ;
extern int    tdebug ;
extern void NoInit( /* */ );
extern nptr step( /*  stop_time */ );
extern char  switch_state[ /*NTTYPES*/ ][ 4 ] ;
#define	 compute_trans_state( TRANS )					\
    ( ((TRANS)->ttype & GATELIST) ?					\
	ComputeTransState( TRANS ):					\
	switch_state[ BASETYPE( (TRANS)->ttype ) ][ (TRANS)->gate->npot ] )
extern int ComputeTransState( /*  t */ );

	/* EXPORTS FROM fio.c */

extern char *fgetline( /*  bp, len, fp */ );
extern int Fread( /*  ptr, size, fp */ );
extern int Fwrite( /*  ptr, size, fp */ );

	/* EXPORTS FROM hist.c */

extern hptr   freeHist ;
extern  hptr   last_hist;
extern int    num_edges ;
extern int    num_punted ;
extern int    num_cons_punted ;
extern hptr   first_model;
extern Ulong  max_time;
extern void init_hist( /* */ );
extern int AddHist( /*  node, value, inp, time, delay, rtime */ );
extern int AddPunted( /*  node, ev, tim */ );
extern void FreeHistList( /*  node */ );
extern void NoMoreIncSim( /* */ );
extern void NewModel( /*  model_num */ );
extern void NewEdge( /*  nd, ev */ );
extern void DeleteNextEdge( /*  nd */ );
extern void FlushHist( /*  ftime */ );
extern int backToTime( /*  nd */ );
extern void DumpHist( /*  fname */ );
extern void ReadHist( /*  fname */ );

	/* EXPORTS FROM intr.c */

extern int    int_received ;
extern void InitSignals( /* */ );

	/* EXPORTS FROM incsim.c */

extern long    INC_RES ;
extern nptr    inc_cause ;
extern long    nevals ;
extern long    i_nevals ;
extern long    nreval_ev ;
extern long    npunted_ev ;
extern long    nstimuli_ev ;
extern long    ncheckpt_ev ;
extern long    ndelaychk_ev ;
extern long    ndelay_ev ;
extern void incsim( /*  ch_list */ );

	/* EXPORTS FROM mem.c */

typedef union MElem
  {
    union MElem  *next;		/* points to next element in linked list */
    int          align[1];	/* dummy used to force word alignment */
  } *MList;
extern char *Falloc( /*  nbytes */ );
extern void Ffree( /*  p, nbytes */ );
extern MList MallocList( /*  nbytes */ );
extern void Vfree( /*  ptr */ );
extern char *Valloc( /*  nbytes */ );

	/* EXPORTS FROM prints.c */

extern void lprintf( /*  va_alist */ );
extern void error( /*  va_alist */ );

	/* EXPORTS FROM netchange.c */

extern nptr rd_changes( /*  fname, logname */ );

	/* EXPORTS FROM network.c */

extern iptr  hinputs ;
extern iptr  linputs ;
extern iptr  uinputs ;
extern iptr  xinputs ;
extern iptr  o_hinputs ;
extern iptr  o_linputs ;
extern iptr  o_uinputs ;
extern iptr  infree ;
extern iptr  *listTbl[ /*8*/ ];
extern FILE  *sfile;
extern void init_listTbl( /* */ );
extern void idelete( /*  n, list */ );
extern void iinsert( /*  n, list */ );
extern void ClearInputs( /* */ );
extern int setin( /*  n, which */ );
extern int wr_state( /*  fname */ );
extern int rd_restore( /*  n */ );
extern int rd_value( /*  n */ );
extern int rd_state( /*  fname, fun */ );
extern int info( /*  n, which */ );

	/* EXPORTS FROM newrstep.c */

extern int       tunitdelay ;
extern int       tdecay ;
extern char      withdriven;
extern int cy_new_val( /*  n */ );
extern void InitThevs( /* */ );

	/* EXPORTS FROM nsubrs.c */

extern char  *ttype[ /*NTTYPES*/ ] ;
extern int GetHashSize( /* */ );
extern int str_eql( /*  s1, s2 */ );
extern int str_match( /*  p, s */ );
extern nptr find( /*  name */ );
extern nptr GetNode( /*  name */ );
extern void n_insert( /*  nd */ );
extern void n_delete( /*  nd */ );
extern int walk_net( /*  fun */ );
extern void walk_net_index( /*  fun, all_nodes */ );
extern nptr GetNodeList( /* */ );
extern nptr Index2node( /*  major, minor */ );
extern void Node2index( /*  nd, major, minor */ );
extern int match_net( /*  pattern, fun, arg */ );
extern int numberp( /*  s */ );
#define	pnode( NODE )	( (NODE)->nname )
extern void init_hash( /* */ );
extern void rm_aliases( /*  nd */ );

	/* EXPORTS FROM parallel.c */

extern void make_parallel( /*  nlist, cl */ );
extern int DestroyParallel( /*  t, width, length */ );
extern int pParallelTxtors( /* */ );
extern void WriteParallel( /*  f */ );
extern void ReadParallel( /*  f */ );
extern void AssignOredTrans( /* */ );

	/* EXPORTS FROM rsim.c */

extern iptr     wlist ;
extern char     *filename;
extern int      analyzerON ;
extern long     sim_time0 ;
extern FILE     *logfile ;
extern void rm_from_vectors( /*  node */ );
extern long GetStepSize( /* */ );
extern void rm_from_seq( /*  node */ );
extern void rm_from_clock( /*  node */ );
#define	DEBUG_EV		0x01		/* event scheduling */
#define	DEBUG_DC		0x02		/* final value computation */
#define	DEBUG_TAU		0x04		/* tau/delay computation */
#define	DEBUG_TAUP		0x08		/* taup computation */
#define	DEBUG_SPK		0x10		/* spike analysis */
#define	DEBUG_TW		0x20		/* tree walk */
#define	LIN_MODEL	0
#define	SWT_MODEL	1
extern main( /*  argc, argv */ );

	/* EXPORTS FROM sched.c */

extern int    debug ;
extern long   cur_delta;
extern nptr   cur_node;
extern long   nevent;
extern evptr  evfree ;
extern int    npending;
extern int    queue_final ;
extern long last_event_time( /* */ );
extern int pending_events( /*  delta, list, end_of_list */ );
extern evptr get_next_event( /*  stop_time */ );
#define free_from_node( ev, nd )					\
  {									\
    if( (nd)->events == (ev) )						\
	(nd)->events = (ev)->nlink;					\
    else								\
      {									\
	register evptr  evp;						\
	for( evp = (nd)->events; evp->nlink != (ev); evp = evp->nlink );\
	evp->nlink = (ev)->nlink;					\
      }									\
  }									\

extern void free_event( /*  event */ );
extern void enqueue_event( /*  n, newvalue, delta, rtime */ );
extern void enqueue_input( /*  n, newvalue */ );
extern void init_event( /* */ );
extern void PuntEvent( /*  node, ev */ );
extern void back_sim_time( /*  btime, is_inc */ );
extern int EnqueueHist( /*  nd, hist, type */ );
extern void DequeueEvent( /*  nd */ );
extern void DelayEvent( /*  ev, delay */ );
extern void EnqueueModelChange( /*  time */ );
extern void rm_inc_events( /* */ );

	/* EXPORTS FROM sim.c */

extern nptr   VDD_node;
extern nptr   GND_node;
extern int    nnodes;
extern int    naliases;
extern int    ntrans[ /* NTTYPES */ ];
extern lptr   freeLinks ;
extern tptr   freeTrans ;
extern MList  freeNodes ;
extern tptr   tcap_list ;
extern int    tcap_cnt ;
extern tptr   ored_list ;
#define	MIN_CAP		0.0005		/* minimum node capacitance (in pf) */
extern int rd_network( /*  simfile */ );
extern void pTotalNodes( /* */ );
extern void pTotalTxtors( /* */ );
extern void ConnectNetwork( /* */ );

	/* EXPORTS FROM sstep.c */

extern int snew_val( /*  n, tinput */ );

	/* EXPORTS FROM stack.c */

extern int     stack_txtors ;
extern void pStackedTxtors( /* */ );
extern void make_stacks( /*  nlist */ );
extern void DestroyStack( /*  stack */ );
extern tptr AddStack( /*  t1 */ );
extern tptr UndoStack( /*  t1 */ );

	/* EXPORTS FROM tpos.c */

extern int    txt_coords ;
extern void EnterPos( /*  tran */ );
extern tptr FindTxtorPos( /*  x, y */ );
extern void DeleteTxtorPos( /*  tran */ );
extern void ChangeTxtorPos( /*  tran, x, y */ );
extern nptr FindNode_TxtorPos( /*  s */ );

	/* EXPORTS FROM usage.c */

extern void InitUsage( /* */ );
extern void set_usage( /* */ );
extern int print_usage( /*  partial, dest */ );
extern void InitUsage( /* */ );
extern void set_usage( /* */ );
extern int print_usage( /*  partial, dest */ );

	/* EXPORTS FROM version.c */

extern char    version[ /**/ ] ;
