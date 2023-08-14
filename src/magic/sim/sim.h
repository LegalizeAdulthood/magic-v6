extern	char	*SimGetNodeCommand();
extern	char	*SimGetNodeName();
extern	char	*SimSelectNode();
extern	bool	SimGetReplyLine();
extern		SimRsimIt();
extern	bool	efPreferredName();
extern  void	SimRsimHandler();
extern  void	SimInit();

extern  bool	SimRecomputeSel;
extern 	bool	SimInitGetnode;
extern	bool	SimGetnodeAlias;
extern 	bool	SimSawAbortString;
extern 	bool	SimRsimRunning;
extern	bool	SimIsGetnode;
extern	bool	SimHasCoords;
extern	bool	SimUseCoords;

extern HashTable SimNodeNameTbl;
extern HashTable SimGNAliasTbl;
extern HashTable SimGetnodeTbl;
extern HashTable SimAbortSeenTbl;

