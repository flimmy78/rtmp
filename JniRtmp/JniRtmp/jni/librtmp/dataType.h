
#ifndef __DATATYPE_H_
#define __DATATYPE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(__LP64__) || \
        defined(__x86_64__) || defined(__ppc64__)
#define COMPILE_64BITS
#endif


typedef unsigned char		BOOLEAN;
//typedef bool				BOOLEAN;
typedef unsigned char		INT8U;                    /* Unsigned  8 bit quantity*/
typedef signed   char		INT8S;                    /* Signed    8 bit quantity*/
typedef unsigned short		INT16U;                   /* Unsigned 16 bit quantity*/
typedef signed   short		INT16S;                   /* Signed   16 bit quantity*/
typedef unsigned int		INT32U;                   /* Unsigned 32 bit quantity*/
typedef unsigned int		uint32_t;                   /* Unsigned 32 bit quantity*/

typedef signed   int		INT32S;                   /* Signed   32 bit quantity*/
typedef double             INT32D;
#if defined(COMPILE_64BITS)
typedef unsigned long 	    INT64U;					  /* Unsigned 64 bit quantity*/
typedef long  			    INT64S;					  /* Unsigned 64 bit quantity*/
#else
typedef unsigned long long	INT64U;					  /* Unsigned 64 bit quantity*/
typedef long long 			INT64S;					  /* Unsigned 64 bit quantity*/
#endif
typedef float				FP32;                     /* Single precision floating point*/
typedef double				FP64;                     /* Double precision floating point*/

/* 64 and 32 bits compile*/
#if defined(COMPILE_64BITS)
	typedef INT64U	   STREAMER_HANDLE;
    typedef INT64U    STREAMER_WPARAM;
    typedef INT64U    STREAMER_LPARAM;
#else
	typedef INT32U	   STREAMER_HANDLE;
    typedef INT32U    STREAMER_WPARAM;
    typedef INT32U    STREAMER_LPARAM;
#endif 
			  

#if defined(WIN32)
typedef char				CHAR;					  /* Signed  8 bit quantity*/
#else
typedef signed char			CHAR;					  /* Signed  8 bit quantity*/
#endif 



/* Define data types for backward compatibility ...   */
/*
#define BYTE				INT8S                     
#define UBYTE				INT8U                     
#define WORD				INT16S                    
#define UWORD				INT16U
#define LONG				INT32S
#define ULONG				INT32U
*/
#ifndef STREAMER_NULL
#define STREAMER_NULL	 0 
#endif 

#ifndef STREAMER_FALSE
#define  STREAMER_FALSE 0
#endif

#ifndef STREAMER_TRUE
#define STREAMER_TRUE	1
#endif 


#define  FALSE         0
#define  TRUE	       1


#ifdef __cplusplus
}
#endif

#endif //__DATATYPE_H_

