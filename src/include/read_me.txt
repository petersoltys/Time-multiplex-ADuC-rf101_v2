Libraries downloaded from dedicated CD to radio modules ADucRF101MKxZ
version : 	1.1
		
changes : 	renamed include.h => library.h

			initial values of variables in radioeng.c
			from =>
			static RIE_BOOL             bPacketTx                     = RIE_FALSE;
			static RIE_BOOL             bPacketRx                     = RIE_FALSE;
			to =>
			static RIE_BOOL             bPacketTx                     = RIE_TRUE;
			static RIE_BOOL             bPacketRx                     = RIE_TRUE;
			