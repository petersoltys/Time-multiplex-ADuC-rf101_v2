Libraries downloaded from dedicated CD to radio modules ADucRF101MKxZ
libraries containing inerface driver functions

version :   1.1
author  :   Analog Devices
        
changes :   renamed include.h => library.h

            initial values of variables in radioeng.c
            from =>
            static RIE_BOOL             bPacketTx                     = RIE_FALSE;
            static RIE_BOOL             bPacketRx                     = RIE_FALSE;
            to =>
            static RIE_BOOL             bPacketTx                     = RIE_TRUE;
            static RIE_BOOL             bPacketRx                     = RIE_TRUE;
            