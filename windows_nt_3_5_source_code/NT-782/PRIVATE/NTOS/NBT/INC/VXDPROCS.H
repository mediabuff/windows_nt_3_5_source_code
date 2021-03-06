/**********************************************************************/
/**           Microsoft Windows/NT               **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    vxdprocs.h

    This file contains VxD specific types/manifests for the NBT driver


    FILE HISTORY:
        Johnl   29-Mar-1993     Created

*/

#ifndef _VXDPROCS_H_
#define _VXDPROCS_H_

//--------------------------------------------------------------------
//
//  Define some ndis stuff here because tdivxd.h needs it however we can't
//  include ndis3\inc\ndis.h because it conflicts with ntconfig.h and we
//  can't take out ntconfig.h because it has definitions needed by other
//  header files...grrrr....
//

#ifdef CHICAGO
#ifndef NDIS_STDCALL
#define NDIS_STDCALL    1
#endif
#endif

#ifdef NDIS_STDCALL
#define NDIS_API __stdcall
#else
#define NDIS_API
#endif

//
// Ndis Buffer
//

#define BUFFER_POOL_SIGN  (UINT)0X4C50424E  /* NBPL */
#define BUFFER_SIGN       (UINT)0x4655424e  /* NBUF */

typedef INT NDIS_SPIN_LOCK, * PNDIS_SPIN_LOCK;

struct _NDIS_BUFFER;
typedef struct _NDIS_BUFFER_POOL {
    UINT Signature;                     //character signature for debug "NBPL"
    NDIS_SPIN_LOCK SpinLock;            //to serialize access to the buffer pool
    struct _NDIS_BUFFER *FreeList;      //linked list of free slots in pool
    UINT BufferLength;                  //amount needed for each buffer descriptor
    UCHAR Buffer[1];                    //actual pool memory
    } NDIS_BUFFER_POOL, * PNDIS_BUFFER_POOL;

#ifdef NDIS_STDCALL
typedef struct _NDIS_BUFFER {
    struct _NDIS_BUFFER *Next;          //pointer to next buffer descriptor in chain
    PVOID VirtualAddress;               //linear address of this buffer
    PNDIS_BUFFER_POOL Pool;             //pointer to pool so we can free to correct pool
    UINT Length;                        //length of this buffer
    UINT Signature;                     //character signature for debug "NBUF"
} NDIS_BUFFER, * PNDIS_BUFFER;

#else

typedef struct _NDIS_BUFFER {
    UINT Signature;                     //character signature for debug "NBUF"
    struct _NDIS_BUFFER *Next;          //pointer to next buffer descriptor in chain
    PVOID VirtualAddress;               //linear address of this buffer
    PNDIS_BUFFER_POOL Pool;             //pointer to pool so we can free to correct pool
    UINT Length;                        //length of this buffer
} NDIS_BUFFER, * PNDIS_BUFFER;
#endif

#define NDIS_STATUS_SUCCESS         0   // Used by CTEinitBlockStruc macro

//
// Possible data types
//

typedef enum _NDIS_PARAMETER_TYPE {
    NdisParameterInteger,
    NdisParameterHexInteger,
    NdisParameterString,
    NdisParameterMultiString
} NDIS_PARAMETER_TYPE, *PNDIS_PARAMETER_TYPE;

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PUCHAR Buffer;
} STRING, *PSTRING;

typedef STRING NDIS_STRING, *PNDIS_STRING;
typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;

//
// To store configuration information
//
typedef struct _NDIS_CONFIGURATION_PARAMETER {
    NDIS_PARAMETER_TYPE ParameterType;
    union {
    ULONG IntegerData;
    NDIS_STRING StringData;
    } ParameterData;
} NDIS_CONFIGURATION_PARAMETER, *PNDIS_CONFIGURATION_PARAMETER;

typedef ULONG NDIS_STATUS;
typedef NDIS_STATUS *PNDIS_STATUS;

VOID NDIS_API
NdisOpenProtocolConfiguration(
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_HANDLE    ConfigurationHandle,
    IN  PNDIS_STRING    ProtocolName
    );

VOID NDIS_API
NdisReadConfiguration(
    OUT PNDIS_STATUS Status,
    OUT PNDIS_CONFIGURATION_PARAMETER *ParameterValue,
    IN NDIS_HANDLE ConfigurationHandle,
    IN PNDIS_STRING Parameter,
    IN NDIS_PARAMETER_TYPE ParameterType
    );

VOID NDIS_API
NdisCloseConfiguration(
    IN NDIS_HANDLE ConfigurationHandle
    );

//--------------------------------------------------------------------

#include <tdivxd.h>
#include <tdistat.h>

//--------------------------------------------------------------------
//
//  Initializes a TA_NETBIOS_ADDRESS structure
//
//     ptanb - Pointer to the TA_NETBIOS_ADDRESS
//     pName - Pointer to the netbios name this address structure represents
//
#define InitNBAddress( ptanb, pName )                                 \
{                                                                     \
    (ptanb)->TAAddressCount           = 1 ;                           \
    (ptanb)->Address[0].AddressLength = sizeof( TDI_ADDRESS_NETBIOS );\
    (ptanb)->Address[0].AddressType   = TDI_ADDRESS_TYPE_NETBIOS ;    \
    (ptanb)->Address[0].Address[0].NetbiosNameType = 0 ;              \
    CTEMemCopy( (ptanb)->Address[0].Address[0].NetbiosName,           \
                pName,                                                \
                NCBNAMSZ ) ;                                          \
}

//
//  Initializes a TDI_CONNECTION_INFORMATION structure for Netbios
//
//      pConnInfo - Pointer to TDI_CONNECTION_INFORMATION structure
//      ptanb - same as for InitNBAddress
//      pName - same as for InitNBAddress
//
#define InitNBTDIConnectInfo( pConnInfo, ptanb, pName )               \
{                                                                     \
    InitNBAddress( ((PTA_NETBIOS_ADDRESS)ptanb), (pName) ) ;          \
    (pConnInfo)->RemoteAddressLength = sizeof( TA_NETBIOS_ADDRESS ) ; \
    (pConnInfo)->RemoteAddress       = (ptanb) ;                      \
}

//
//  Initializes an NDIS buffer (doesn't allocate memory)
//
//      pndisBuff - Pointer to NDIS buffer to initialize
//      pvData    - Pointer to buffer data
//      cbLen     - Length of user data (in bytes)
//      pndisBuffnext - Next NDIS buffer in chain (or NULL if last)
//
#define InitNDISBuff( pndisBuff, pvData, cbLen, pndisBuffNext )       \
{                                                                     \
    (pndisBuff)->Signature      = BUFFER_SIGN ;                       \
    (pndisBuff)->Next           = (pndisBuffNext) ;                   \
    (pndisBuff)->Length         = (cbLen) ;                           \
    (pndisBuff)->VirtualAddress = (pvData) ;                          \
    (pndisBuff)->Pool           = NULL ;                              \
}

//
//  Proper NCB error type
//
typedef uchar NCBERR ;

//
//  This is a private NCB command used for adding name number 0 to the
//  name table.  It is submitted directly by the Nbt driver during
//  initialization.  Note that if a client tries to submit an NCB with
//  this command we'll return illegal command.
//
#define NCBADD_PERMANENT_NAME       0xff

//
//  Last valid NCB session or name number
//
#define MAX_NCB_NUMS                254

//
//  When a send or receive tick count reaches this value, it's timed out
//
#define NCB_TIMED_OUT                 1

//
//  A timeout of this value means the NCB will never timeout
//
#define NCB_INFINITE_TIME_OUT         0

//--------------------------------------------------------------------
//
//  Receieve session data context, set in VxdReceive.
//  Allocated on the heap (too big for ncb_reserve).
//
#define RCVCONT_SIGN                    0x1900BEEF
typedef struct _RCV_CONTEXT
{
    union
    {
        LIST_ENTRY         ListEntry ;  // Used when NCB is put on RcvHead
        EventRcvBuffer     evrcvbuf ;   // Used for doing actual receive
                                        // (after removed from RcvHead)
    } ;
    UINT               Signature ;
    tLOWERCONNECTION * pLowerConnId ;   // Where data is arriving from
    NCB *              pNCB ;           // Pointer to NCB
    NDIS_BUFFER        ndisBuff ;       // Transport fills this buffer
    UCHAR              RTO ;            // 1/2 second ticks till timeout
} RCV_CONTEXT, *PRCV_CONTEXT ;

//
//  Allocate, initialize and free a receive context structure
//
#define GetRcvContext( ppContext )                                        \
    (STATUS_SUCCESS == NbtGetBuffer( &NbtConfig.RcvContextFreeList,       \
                       (PLIST_ENTRY*)ppContext,                           \
                       eNBT_RCV_CONTEXT ))

#define FreeRcvContext( pRcvContext )                          \
{                                                              \
    ASSERT( (pRcvContext)->Signature == RCVCONT_SIGN ) ;       \
    InsertTailList( &NbtConfig.RcvContextFreeList,             \
                    &(pRcvContext)->ListEntry ) ;              \
}

#define InitRcvContext(  pRcvCont, pRcvLowerConn, pRcvNCB ) \
{                                                        \
    pRcvCont->Signature   = RCVCONT_SIGN ;               \
    pRcvCont->pLowerConnId= pRcvLowerConn ;              \
    pRcvCont->pNCB        = pRcvNCB ;                    \
}

//--------------------------------------------------------------------
//
//  Send session data context, set in VxdSend.
//  Stored in ncb_reserve
//
typedef struct _SEND_CONTEXT
{
    LIST_ENTRY         ListEntry ;      // Kept on timeout queue
    tSESSIONHDR      * pHdr ;           // Allocated session header
    UCHAR              STO ;            // 1/2 second ticks till timeout
} SEND_CONTEXT, *PSEND_CONTEXT ;


#define GetSessionHdr( ppHdr )                                            \
    (STATUS_SUCCESS == NbtGetBuffer( &NbtConfig.SessionBufferFreeList,    \
                                     (PLIST_ENTRY*)ppHdr,                 \
                                     eNBT_SESSION_HDR ))

#define FreeSessionHdr( pSessionHdr )                          \
{                                                              \
    InsertTailList( &NbtConfig.SessionBufferFreeList,          \
                    (PLIST_ENTRY) pSessionHdr ) ;              \
}

//--------------------------------------------------------------------
//
//  TDI Send context (used by TdiSend)
//
//  When handling the datagram completion routines, we need to set up
//  another completion routine.  We store the old completion routine
//  in this structure
//
typedef union _TDI_SEND_CONTEXT
{
    LIST_ENTRY     ListEntry ;         // Only used when on buffer free list

    struct
    {
        PVOID          NewContext ;
        NBT_COMPLETION OldRequestNotifyObject ;
        PVOID          OldContext ;
        NDIS_BUFFER    ndisHdr ;       // Generally NBT message
        NDIS_BUFFER    ndisData1 ;     // Data or SMB
        NDIS_BUFFER    ndisData2 ;     // Data if ndisData1 is an SMB
    } ;
} TDI_SEND_CONTEXT, * PTDI_SEND_CONTEXT ;

//
//  Allocates a TDI_SEND_CONTEXT
//
#define GetSendContext( ppContext )                                        \
    (STATUS_SUCCESS == NbtGetBuffer( &NbtConfig.SendContextFreeList,       \
                       (PLIST_ENTRY*)ppContext,                            \
                       eNBT_SEND_CONTEXT ))

//
//  Frees a send context structure and its allocated memory
//
#define FreeSendContext( psendCont )                           \
{                                                              \
    InsertTailList( &NbtConfig.SendContextFreeList,            \
                    &(psendCont)->ListEntry ) ;                \
}

//--------------------------------------------------------------------
//
//  Lana related stuff
//

#define NBT_MAX_LANAS     8

typedef struct
{
    tDEVICECONTEXT * pDeviceContext ;   // Adapter for this Lana
} LANA_ENTRY, *PLANA_ENTRY ;

extern LANA_ENTRY LanaTable[NBT_MAX_LANAS] ;

//--------------------------------------------------------------------
//
//  Procedures in ncb.c
//
//
NCBERR MapTDIStatus2NCBErr( TDI_STATUS status ) ;

//
//  Get the correct adapter for this NCBs Lana
//
tDEVICECONTEXT * GetDeviceContext( NCB * pNCB ) ;

extern BOOL fNCBCompleted ;    // Wait NCB completed before returning to submitter
extern BOOL fWaitingForNCB ;   // We are blocked waiting for a Wait NCB to complete
extern CTEBlockStruc WaitNCBBlock ;  // Wait on this until signaled in completion
extern UCHAR LanaBase ;

#define IPINFO_BUFF_SIZE  (sizeof(IPInfo) + MAX_IP_NETS * sizeof(NetInfo))

//--------------------------------------------------------------------
//
//  TDI Dispatch table (exported from vtdi.386)
//
extern TDIDispatchTable * TdiDispatch ;

//
//  Wrappers for interfacing to the TDI Dispatch table
//
#define TdiVxdOpenAddress           TdiDispatch->TdiOpenAddressEntry
#define TdiVxdCloseAddress          TdiDispatch->TdiCloseAddressEntry
#define TdiVxdOpenConnection        TdiDispatch->TdiOpenConnectionEntry
#define TdiVxdCloseConnection       TdiDispatch->TdiCloseConnectionEntry
#define TdiVxdAssociateAddress      TdiDispatch->TdiAssociateAddressEntry
#define TdiVxdDisAssociateAddress   TdiDispatch->TdiDisAssociateAddressEntry
#define TdiVxdConnect               TdiDispatch->TdiConnectEntry
#define TdiVxdDisconnect            TdiDispatch->TdiDisconnectEntry
#define TdiVxdListen                TdiDispatch->TdiListenEntry
#define TdiVxdAccept                TdiDispatch->TdiAcceptEntry
#define TdiVxdReceive               TdiDispatch->TdiReceiveEntry
#define TdiVxdSend                  TdiDispatch->TdiSendEntry
#define TdiVxdSendDatagram          TdiDispatch->TdiSendDatagramEntry
#define TdiVxdReceiveDatagram       TdiDispatch->TdiReceiveDatagramEntry
#define TdiVxdSetEventHandler       TdiDispatch->TdiSetEventEntry
#define TdiVxdQueryInformationEx    TdiDispatch->TdiQueryInformationExEntry
#define TdiVxdSetInformationEx      TdiDispatch->TdiSetInformationExEntry

//--------------------------------------------------------------------
//
//  NTSTATUS to TDI_STATUS mappings.
//
//  Rather then convert from NTSTATUS to TDI_STATUS (then sometimes back to
//  NTSTATUS) we'll just use TDI_STATUS codes everywhere (and map to NCBERR
//  when returning codes to the Netbios interface).
//
#undef STATUS_SUCCESS
#undef STATUS_INSUFFICIENT_RESOURCES
#undef STATUS_ADDRESS_ALREADY_EXISTS
#undef STATUS_TOO_MANY_ADDRESSES
#undef STATUS_INVALID_ADDRESS
#undef STATUS_BUFFER_OVERFLOW
#undef STATUS_TRANSACTION_INVALID_TYPE
#undef STATUS_TRANSACTION_INVALID_ID
#undef STATUS_EVENT_DONE
#undef STATUS_TRANSACTION_TIMED_OUT
#undef STATUS_EVENT_PENDING
#undef STATUS_PENDING
#undef STATUS_BAD_NETWORK_NAME
#undef STATUS_REQUEST_NOT_ACCEPTED
#undef STATUS_INVALID_CONNECTION
#undef STATUS_DATA_NOT_ACCEPTED
#undef STATUS_MORE_PROCESSING_REQUIRED
#undef STATUS_IO_TIMEOUT
#undef STATUS_TIMEOUT
#undef STATUS_GRACEFUL_DISCONNECT
#undef STATUS_CONNECTION_RESET

#define STATUS_SUCCESS                    TDI_SUCCESS
//#define STATUS_UNSUCCESSFUL
#define STATUS_MORE_PROCESSING_REQUIRED   TDI_MORE_PROCESSING
#define STATUS_BAD_NETWORK_NAME           TDI_INVALID_CONNECTION
#define STATUS_DATA_NOT_ACCEPTED          TDI_NOT_ACCEPTED
//#define STATUS_REMOTE_NOT_LISTENING
//#define STATUS_DUPLICATE_NAME
//#define STATUS_INVALID_PARAMETER
//#define STATUS_OBJECT_NAME_COLLISION    Duplicate Name
//#define STATUS_SHARING_VIOLATION        Duplicate Name
#define STATUS_CONNECTION_INVALID         TDI_INVALID_CONNECTION
#define STATUS_INVALID_CONNECTION         TDI_INVALID_CONNECTION
#define STATUS_INSUFFICIENT_RESOURCES     TDI_NO_RESOURCES
#define STATUS_ADDRESS_ALREADY_EXISTS     TDI_ADDR_IN_USE
#define STATUS_TOO_MANY_ADDRESSES         TDI_NO_FREE_ADDR
#define STATUS_INVALID_ADDRESS            TDI_ADDR_INVALID
#define STATUS_BUFFER_OVERFLOW            TDI_BUFFER_OVERFLOW
#define STATUS_TRANSACTION_INVALID_TYPE   TDI_BAD_EVENT_TYPE
#define STATUS_TRANSACTION_INVALID_ID     TDI_BAD_OPTION     // ??
#define STATUS_EVENT_DONE                 TDI_EVENT_DONE
#define STATUS_TRANSACTION_TIMED_OUT      TDI_TIMED_OUT
#define STATUS_IO_TIMEOUT                 TDI_TIMED_OUT
#define STATUS_TIMEOUT                    TDI_TIMED_OUT
#define STATUS_EVENT_PENDING              TDI_PENDING
#define STATUS_PENDING                    TDI_PENDING
#define STATUS_GRACEFUL_DISCONNECT        TDI_GRACEFUL_DISC
#define STATUS_CONNECTION_RESET           TDI_CONNECTION_RESET

//
//  This is the "Name deregistered but not deleted because of
//  active sessions" error code.
//
#define STATUS_NRC_ACTSES                 0xCA000001

//
//  The NT_SUCCESS macro looks at the high bytes of the errr code which isn't
//  appropriate for our mapping to TDI_STATUS error codes
//
#undef NT_SUCCESS
#define NT_SUCCESS(err)                   ((err==TDI_SUCCESS)||(err==TDI_PENDING))

//--------------------------------------------------------------------
//
//  General porting macros
//
//
//--------------------------------------------------------------------

//
//  Note that the ExInterlocked* routines (in ntos\ex\i386) do a spinlock
//  for MP machines.  Since we aren't MP we shouldn't need the spin lock.
//  We shouldn't need to disable interrupts either.
//
#define ExInterlockedInsertTailList(list, entry, spinlock )     \
            InsertTailList( (list), (entry) )

#define ExInterlockedInsertHeadList(list, entry, spinlock )     \
            InsertHeadList( (list), (entry) )

//
//  These two definitions must be kept keep a couple of NT macros use
//  the ExInterlocked* macros
//
#ifdef ExInterlockedIncrementLong
#undef ExInterlockedIncrementLong
#endif
#define ExInterlockedIncrementLong(n,spinlock)                  \
            CTEInterlockedIncrementLong( n, spinlock )

#ifdef ExInterlockedDecrementLong
#undef ExInterlockedDecrementLong
#endif
#define ExInterlockedDecrementLong(n,spinlock)                  \
            CTEInterlockedDecrementLong( n, spinlock )

//--------------------------------------------------------------------
//
//  Debug helper macros
//
#undef  ASSERT
#undef  ASSERTMSG
#ifdef DEBUG
    #include <vxddebug.h>

    extern DWORD DebugFlags ;
    extern char  DBOut[4096] ;
    extern int   iCurPos ;
    void   NbtDebugOut( char * ) ;

    #define DBGFLAG_ALL            (0x00000001)     // Everything else
    #define DBGFLAG_LMHOST         (0x00000002)
    #define DBGFLAG_KDPRINTS       (0x00000004)     // Jim's KdPrint output
    #define DBGFLAG_ERROR          (0x00000010)
    #define DBGFLAG_AUX_OUTPUT     (0x00000080)

    #define DbgPrint( s )                   \
       if ( DebugFlags & DBGFLAG_ALL )      \
       {                                    \
          VxdSprintf( DBOut+iCurPos, s ) ;  \
          NbtDebugOut( DBOut+iCurPos ) ;    \
       }

    #define DbgPrintNum( n )                      \
       if ( DebugFlags & DBGFLAG_ALL )            \
       {                                          \
          VxdSprintf( DBOut+iCurPos, "%x", n ) ;  \
          NbtDebugOut( DBOut+iCurPos ) ;          \
       }

    #undef KdPrint
    #define KdPrint( s )                          \
       if ( DebugFlags & DBGFLAG_KDPRINTS )       \
       {                                          \
           VxdPrintf s ;                          \
       }

    //
    //  Conditional print routines
    //
    #define CDbgPrint( flag, s )            \
       if ( DebugFlags & (flag) )           \
       {                                    \
          VxdSprintf( DBOut+iCurPos, s ) ;  \
          NbtDebugOut( DBOut+iCurPos ) ;    \
       }

    #define CDbgPrintNum( flag, n )               \
       if ( DebugFlags & (flag) )                 \
       {                                          \
          VxdSprintf( DBOut+iCurPos, "%x", n ) ;  \
          NbtDebugOut( DBOut+iCurPos ) ;          \
       }

    #define DbgBreak()             _asm int 3
    #define ASSERT( exp )          CTEAssert( exp )
    #define ASSERTMSG( msg, exp )  CTEAssert( exp )
    //
    //  REQUIRE is an ASSERT that keeps the expression under non-debug
    //  builds
    //
    #define REQUIRE( exp )         ASSERT( exp )

    //
    //  Consistency checks of the interrupt vector table to help watch
    //  for NULL pointer writes
    //
    extern BYTE abVecTbl[256] ;
    #define INIT_NULL_PTR_CHECK()  memcpy( abVecTbl, 0, sizeof( abVecTbl ))
    #define CHECK_MEM()            if ( sizeof(abVecTbl) != VxdRtlCompareMemory( 0, abVecTbl, sizeof(abVecTbl)) ) \
                                   {                                                         \
                                       DbgPrint("Vector table corrupt at " ) ;               \
                                       DbgPrintNum( VxdRtlCompareMemory( 0, abVecTbl, sizeof(abVecTbl) ) ) ;\
                                       DbgPrint("\n\r") ;                                    \
                                       _asm int 3                                            \
                                   }                                                         \
                                   CTECheckMem(__FILE__) ;
#else
    #define DbgPrint( s )
    #define DbgPrintNum( n )
    #define DbgBreak()
    #undef KdPrint
    #define KdPrint( s )
    #define ASSERT( exp )           { ; }
    #define ASSERTMSG( msg, exp )   { ; }
    #define REQUIRE( exp )          { exp ; }
    #define INIT_NULL_PTR_CHECK()
    #define CHECK_MEM()
    #define CDbgPrint( flag, s )
    #define CDbgPrintNum( flag, n )
#endif

//---------------------------------------------------------------------
//
// FROM tdihndlr.c
//
TDI_STATUS
TdiReceiveHandler (
    IN PVOID ReceiveEventContext,
    IN PVOID ConnectionContext,
    IN USHORT ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT PULONG BytesTaken,
    IN PVOID Data,
    EventRcvBuffer * pevrcvbuf
    );

TDI_STATUS
ReceiveAnyHandler (                     //  Handles NCBRCVANY commands, is
    IN PVOID ReceiveEventContext,       //  called after all other receive
    IN PVOID ConnectionContext,         //  handlers
    IN USHORT ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT PULONG BytesTaken,
    IN PVOID Data,
    PVOID * ppBuffer                    // Pointer to ListEntry of RCV_CONTEXT
    ) ;

TDI_STATUS
VxdDisconnectHandler (                  //  Cleans up Netbios stuff for remote
    IN PVOID DisconnectEventContext,    //  disconnects
    IN PVOID ConnectionContext,
    IN PVOID DisconnectData,
    IN ULONG DisconnectInformationLength,
    IN PVOID pDisconnectInformation,
    IN ULONG DisconnectIndicators
    ) ;

VOID
CompletionRcv(
    IN PVOID pContext,
    IN uint tdistatus,
    IN uint BytesRcvd
    );

TDI_STATUS
TdiConnectHandler (
    IN PVOID    pConnectEventContext,
    IN int      RemoteAddressLength,
    IN PVOID    pRemoteAddress,
    IN int      UserDataLength,
    IN PVOID    pUserData,
    IN int      OptionsLength,
    IN PVOID    pOptions,
    IN PVOID  * pAcceptingID,
    IN ConnectEventInfo * pEventInfo
    );

TDI_STATUS
TdiDisconnectHandler (
    PVOID EventContext,
    PVOID ConnectionContext,
    ULONG DisconnectDataLength,
    PVOID DisconnectData,
    ULONG DisconnectInformationLength,
    PVOID DisconnectInformation,
    ULONG DisconnectIndicators      // Is this the Flags field?
    );

TDI_STATUS
TdiRcvDatagramHandler(
    IN PVOID    pDgramEventContext,
    IN int      SourceAddressLength,
    IN PVOID    pSourceAddress,
    IN int      OptionsLength,
    IN PVOID    pOptions,
    IN UINT     ReceiveDatagramFlags,
    IN ULONG    BytesIndicated,
    IN ULONG    BytesAvailable,
    OUT ULONG   *pBytesTaken,
    IN PVOID    pTsdu,
    OUT EventRcvBuffer * * ppBuffer //OUT PIRP    *pIoRequestPacket
    );
TDI_STATUS
TdiRcvNameSrvHandler(
    IN PVOID    pDgramEventContext,
    IN int      SourceAddressLength,
    IN PVOID    pSourceAddress,
    IN int      OptionsLength,
    IN PVOID    pOptions,
    IN UINT     ReceiveDatagramFlags,
    IN ULONG    BytesIndicated,
    IN ULONG    BytesAvailable,
    OUT ULONG   *pBytesTaken,
    IN PVOID    pTsdu,
    OUT EventRcvBuffer * * ppBuffer //OUT PIRP    *pIoRequestPacket
    );
TDI_STATUS
TdiErrorHandler (
    IN PVOID Context,
    IN ULONG Status
    );

NTSTATUS
CompletionRcvDgram(
    IN PVOID      Context,
    IN TDI_STATUS tdistatus,
    IN UINT       RcvdSize
    ) ;

//---------------------------------------------------------------------
//
// FROM init.c
//

PVOID
CTEAllocInitMem(
    IN USHORT cbBuff ) ;

NTSTATUS
VxdReadIniString(
    IN      LPTSTR   pchKeyName,
    IN OUT  LPTSTR * ppStringBuff
    ) ;

NTSTATUS CreateDeviceObject(
    IN  tNBTCONFIG  *pConfig,
    IN  ULONG        IpAddr,
    IN  ULONG        IpMask,
    IN  ULONG        IpNameServer,
    IN  ULONG        IpBackupServer,
    IN  ULONG        IpDnsServer,
    IN  ULONG        IpDnsBackupServer,
    IN  UCHAR        MacAddr[],
    IN  UCHAR        IpIndex
    ) ;

void GetNameServerAddress( ULONG   IpAddr,
                           PULONG  pIpNameServer,
                           UINT    mode);

#define COUNT_NS_ADDR     4         // Maximum number of name server addresses
//---------------------------------------------------------------------
//
// FROM vxdfile.asm
//

HANDLE
VxdFileOpen(
    IN char * pchFile ) ;

ULONG
VxdFileRead(
    IN HANDLE hFile,
    IN ULONG  BytesToRead,
    IN BYTE * pBuff ) ;

VOID
VxdFileClose(
    IN HANDLE hFile ) ;

//---------------------------------------------------------------------
//
// FROM vnbtd.asm
//

ULONG
GetProfileHex(
    IN HANDLE ParametersHandle,     // Not used
    IN PCHAR ValueName,
    IN ULONG DefaultValue,
    IN ULONG MinimumValue
    );

ULONG
GetProfileInt(
    IN HANDLE ParametersHandle,     // Not used
    IN PCHAR ValueName,
    IN ULONG DefaultValue,
    IN ULONG MinimumValue
    );

TDI_STATUS DhcpQueryInfo( UINT Type, PVOID pBuff, UINT * pSize ) ;

//---------------------------------------------------------------------
//
// FROM tdiout.c
//
NTSTATUS VxdDisconnectWait( tLOWERCONNECTION * pLowerConn,
                            tDEVICECONTEXT   * pDeviceContext,
                            ULONG              Flags,
                            PVOID              Timeout) ;

NTSTATUS VxdScheduleDelayedCall( tDGRAM_SEND_TRACKING * pTracker,
                                 PVOID                  pClientContext,
                                 PVOID                  ClientCompletion,
                                 PVOID                  CallBackRoutine ) ;

//---------------------------------------------------------------------
//
// FROM timer.c
//
BOOL CheckForTimedoutNCBs( CTEEvent *pEvent, PVOID pCont ) ;
VOID StopTimeoutTimer( VOID );
NTSTATUS StartRefreshTimer( VOID );

//---------------------------------------------------------------------
//
// FROM tdicnct.c
//
NTSTATUS CloseAddress( HANDLE hAddress ) ;


//---------------------------------------------------------------------
//
// FROM wfw.c - Snowball specific routines
//
#ifndef CHICAGO

BOOL GetActiveLanasFromIP( VOID );

#endif //!CHICAGO


//---------------------------------------------------------------------
//
// FROM chic.c - Chicago specific routines
//
#ifdef CHICAGO

NTSTATUS DestroyDeviceObject(
    tNBTCONFIG  *pConfig,
    ULONG        IpAddr
    );

BOOL IPRegisterAddrChangeHandler( PVOID AddChangeHandler, BOOL );

TDI_STATUS IPNotification( ULONG    IpAddress,
                           ULONG    IpMask,
                           PVOID    pDevNode,
                           BOOL     fNew );

#endif //CHICAGO

//--------------------------------------------------------------------
//
//  Procedures in vxdisol.c
//
//
NCBERR   VxdOpenName( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdCloseName( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdCall( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdListen( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdDgramSend( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdDgramReceive( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdReceiveAny( tDEVICECONTEXT  *pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdReceive( tDEVICECONTEXT  * pDeviceContext, NCB * pNCB   ) ;
NCBERR   VxdHangup( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdCancel( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;
NCBERR   VxdSend( tDEVICECONTEXT  * pDeviceContext, NCB * pNCB   ) ;
NCBERR   VxdSessionStatus( tDEVICECONTEXT * pDeviceContext, NCB * pNCB ) ;

//
//  Flag passed to TdiSend indicating we are dealing with a chain send
//  and not a normal send.
//
#define CHAIN_SEND_FLAG     0x80
typedef struct _tBUFFERCHAINSEND
{
    tBUFFER tBuff ;     // Must be first member of this structure!!
    PVOID   pBuffer2 ;
    ULONG   Length2 ;
} tBUFFERCHAINSEND ;


//
//  Flag for pConnectEle->Flags indicating whether the client has been
//  notified the session is dead (by completing an NCB with NRC_SCLOSED)
//
#define   NB_CLIENT_NOTIFIED    0x01


//
//  Translates the name number/logical session number to the appropriate
//  structure pointer
//
NCBERR   VxdFindClientElement( tDEVICECONTEXT * pDeviceContext,
                               UCHAR            ncbnum,
                               tCLIENTELE   * * ppClientEle,
                               enum CLIENT_TYPE Type ) ;
NCBERR   VxdFindConnectElement( tDEVICECONTEXT * pDeviceContext,
                                NCB            * pNCB,
                                tCONNECTELE  * * ppConnectEle ) ;
NCBERR   VxdFindLSN( tDEVICECONTEXT * pDeviceContext,
                     tCONNECTELE    * pConnectEle,
                     UCHAR          * plsn ) ;
NCBERR   VxdFindNameNum( tDEVICECONTEXT * pDeviceContext,
                         tADDRESSELE    * pAddressEle,
                         UCHAR          * pNum ) ;
//
//  Used by Register/Unregister for selecting either the name table or the
//  session table from the device context
//
typedef enum
{
    NB_NAME,
    NB_SESSION
} NB_TABLE_TYPE ;

BOOL NBRegister( tDEVICECONTEXT * pDeviceContext,
                 UCHAR          * pNCBNum,
                 PVOID            pElem,
                 NB_TABLE_TYPE    NbTable ) ;
BOOL NBUnregister( tDEVICECONTEXT * pDeviceContext,
                   UCHAR            NCBNum,
                   NB_TABLE_TYPE    NbTable ) ;

TDI_STATUS VxdCompleteSessionNcbs( tDEVICECONTEXT * pDeviceContext,
                                   tCONNECTELE    * pConnEle ) ;

NCBERR VxdCleanupAddress( tDEVICECONTEXT * pDeviceContext,
                          NCB            * pNCB,
                          tCLIENTELE     * pClientEle,
                          UCHAR            NameNum,
                          BOOL             fDeleteAddress ) ;

BOOL ActiveSessions( tCLIENTELE * pClientEle ) ;

//
//  This structure holds context information while we are waiting for
//  a session setup to complete (either listen or call)
//
//  It is stored in the ncb_reserve field of the NCB
//
typedef struct _SESS_SETUP_CONTEXT
{
    TDI_CONNECTION_INFORMATION * pRequestConnect ;  //
    TDI_CONNECTION_INFORMATION * pReturnConnect ;   // Name who answered the listen
    tCONNECTELE                * pConnEle ;
    UCHAR                        fIsWorldListen ;   // Listenning for '*'?
} SESS_SETUP_CONTEXT, *PSESS_SETUP_CONTEXT ;


void VxdTearDownSession( tDEVICECONTEXT      * pDevCont,
                         tCONNECTELE         * pConnEle,
                         PSESS_SETUP_CONTEXT   pCont,
                         NCB                 * pNCB ) ;

//
//  Finishes off a Netbios request (fill in NCB fields, call the post
//  routine etc.).  Is macroed as CTEIoComplete.
//
VOID VxdIoComplete( PCTE_IRP pirp, NTSTATUS status, ULONG cbExtra ) ;


#endif //_VXDPROCS_H_

