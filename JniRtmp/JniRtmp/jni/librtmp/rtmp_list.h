#ifndef __RTMP_POLL_H__
#define __RTMP_POLL_H__

#include "config.h"
//#include "utils.h"
#include <sys/types.h>
#include "dataType.h"

#if(RTMP_STREAM_POLL)

#ifdef __cplusplus
extern "C" {
#endif
/****************************************************************************************************/
//  双向列表,需要对外提供
/****************************************************************************************************/

typedef INT32S (*RTMP_OBJECT_COMPARE_FUN) ( void* dst, void* org );
typedef void (*RTMP_OBJECT_FREE_FUN)(void *val);
typedef void*(*RTMP_OBJECT_COPY_FUN)(void* val);
typedef void (*RTMP_OBJECT_UPDATE_FUN)( void* dst, void* org );
typedef CHAR* (*RTMP_OBJECT_GET_KEY_FUN)(void* val);


typedef struct _RTMP_LIST_
{
	struct _RTMP_LIST_	*prev;
	struct _RTMP_LIST_	*next;
	void				*val;  
}RTMP_LIST, *RTMP_LIST_PTR;


RTMP_LIST_PTR rtmp_list_move_first ( RTMP_LIST_PTR list, void **val );
RTMP_LIST_PTR rtmp_list_move_last ( RTMP_LIST_PTR list, void **val );
void*         rtmp_list_set_value ( RTMP_LIST_PTR list, void *val );
RTMP_LIST_PTR rtmp_list_new ( void  *val);
RTMP_LIST_PTR rtmp_list_clone ( RTMP_LIST_PTR list );
RTMP_LIST_PTR rtmp_list_push_front (RTMP_LIST_PTR list, void *val);
RTMP_LIST_PTR rtmp_list_pop_front ( RTMP_LIST_PTR list,  void **val );
RTMP_LIST_PTR rtmp_list_push_back ( RTMP_LIST_PTR list,  void *val );
RTMP_LIST_PTR rtmp_list_pop_back( RTMP_LIST_PTR list,  void **	val );
RTMP_LIST_PTR rtmp_list_insert_before ( RTMP_LIST_PTR list, void *val );
RTMP_LIST_PTR rtmp_list_insert_after ( RTMP_LIST_PTR list, void *val );
void*         rtmp_list_replace ( RTMP_LIST_PTR list, void *val);
RTMP_LIST_PTR rtmp_list_remove ( RTMP_LIST_PTR list, RTMP_OBJECT_FREE_FUN fun);
BOOLEAN       rtmp_list_is_same ( RTMP_LIST_PTR dst,  RTMP_LIST_PTR src );
BOOLEAN       rtmp_list_is_empty (RTMP_LIST_PTR list);
INT32S        rtmp_list_get_count( RTMP_LIST_PTR list ); 
RTMP_LIST_PTR rtmp_list_move_prev( RTMP_LIST_PTR list, void **val ); 
RTMP_LIST_PTR rtmp_list_move_next  ( RTMP_LIST_PTR list, void **val );
void*	      rtmp_list_get_index(RTMP_LIST_PTR list, INT32S index);
void*         rtmp_list_get_val(RTMP_LIST_PTR list, void* val, RTMP_OBJECT_COMPARE_FUN fun);
void*         rtmp_list_get_value ( RTMP_LIST_PTR list );
INT32S        rtmp_list_remove_all ( RTMP_LIST_PTR list, RTMP_OBJECT_FREE_FUN fun);
BOOLEAN       rtmp_list_is_exist(RTMP_LIST_PTR list, void* val, RTMP_OBJECT_COMPARE_FUN fun);
BOOLEAN       rtmp_list_append_ex ( RTMP_LIST_PTR *list, void *val );
BOOLEAN       rtmp_list_append ( RTMP_LIST_PTR *head, RTMP_LIST_PTR tail );
BOOLEAN       rtmp_list_delete_by_val(RTMP_LIST_PTR *head, void* val, 
	RTMP_OBJECT_COMPARE_FUN compare_fun,
	RTMP_OBJECT_FREE_FUN free_fun);
RTMP_LIST_PTR afmobi_list_remove_by_val(RTMP_LIST_PTR *head, void* val, RTMP_OBJECT_COMPARE_FUN compare_fun);



#ifdef __cplusplus
}
#endif

#endif //RTMP_STREAM_POLL

#endif//__RTMP_POLL_H__

