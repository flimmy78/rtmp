
//#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "rtmp_list.h"


#define  RTMP_LIST_SEEK_TO_FIRST(list) \
    while ( (list)->prev!=NULL )  \
    { \
        (list) = (list)->prev; \
    }

#define  RTMP_LIST_SEEK_TO_LAST(list) \
    while ( (list)->next!=NULL )  \
    { \
        (list) = (list)->next; \
    }

#define RTMP_ALLOC     malloc
#define RTMP_FREE      free


#define CHECK_VALUE(x,y,z)\
	if( (x) == (y) )\
	{\
		return (z);\
	}

#define CHECK_INVALID(x, y) CHECK_VALUE((x), NULL, (y))


INT32S  rtmp_list_get_count( RTMP_LIST_PTR list ) 
{
    INT32S length = 0;

	CHECK_INVALID(list, 0);
    while ( NULL != list->prev)
    {
        list = list->prev;
    }

    for ( ;  NULL != list; list = list->next )
    {
        length ++ ;
    }
    return length;
}

RTMP_LIST_PTR rtmp_list_move_prev( RTMP_LIST_PTR list, void **val ) 
{
	CHECK_INVALID(list, NULL);
    list = list->prev;

    if( NULL != list && NULL != val)
    {
        *val = list->val;
    }
    return list;
}
RTMP_LIST_PTR  rtmp_list_move_next  ( RTMP_LIST_PTR list, void **val )
{
	CHECK_INVALID(list, NULL);
    list = list->next;

    if( NULL!= list  &&  NULL != val)
    {
        *val = list->val;
    }
    return list;
}
RTMP_LIST_PTR rtmp_list_move_first ( RTMP_LIST_PTR list, void **val )
{
	CHECK_INVALID(list, NULL);
    RTMP_LIST_SEEK_TO_FIRST(list);

    if( NULL != val)
    {
        *val = list->val;
    }

    return list;
}

RTMP_LIST_PTR rtmp_list_move_last ( RTMP_LIST_PTR list, void **val )
{
	CHECK_INVALID(list, NULL);
    RTMP_LIST_SEEK_TO_LAST(list);
    if( NULL != val)
    {
        *val = list->val;
    }

    return list;
}
void* rtmp_list_get_value ( RTMP_LIST_PTR list )
{
	CHECK_INVALID(list, NULL);
    return list->val;
}
void*  rtmp_list_set_value ( RTMP_LIST_PTR list, void *val )
{
    void *retval;
	
	CHECK_INVALID(list, NULL);
	
    retval = list->val;
    list->val = val;
    return retval;
}

BOOLEAN  rtmp_list_is_empty (RTMP_LIST_PTR list)
{
	CHECK_INVALID(list, TRUE);
    return FALSE;
}

RTMP_LIST_PTR rtmp_list_new ( void  *val)
{
    RTMP_LIST_PTR list ;

    list = (RTMP_LIST_PTR) RTMP_ALLOC( sizeof(RTMP_LIST) );
	CHECK_INVALID(list, NULL);
    list->prev = list->next = NULL;
    list->val  = val;
	return list;
}

void rtmp_list_delete ( RTMP_LIST_PTR list , RTMP_OBJECT_FREE_FUN fun)
{
	if( NULL != fun && NULL != list && NULL != list->val )
	{
		fun(list->val);
	}
	RTMP_FREE(list);
}

RTMP_LIST_PTR  rtmp_list_clone ( RTMP_LIST_PTR list )
{
    RTMP_LIST_PTR list_new = NULL ;
    RTMP_LIST_PTR list_temp = NULL ;

	CHECK_INVALID(list, NULL);
    RTMP_LIST_SEEK_TO_FIRST(list);

    for ( ;  NULL != list; list = list->next )
    {
        list_temp = rtmp_list_push_back ( list_new, list->val ) ;
        if(NULL == list_temp)
        {  
            rtmp_list_remove_all(list_new , NULL);
            return NULL;
        }

        list_new = list_temp;
    }

    return list_new;
}

RTMP_LIST_PTR  rtmp_list_push_front (RTMP_LIST_PTR list, void *val)
{
    RTMP_LIST_PTR clist_new;

    clist_new = rtmp_list_new(val);
	CHECK_INVALID(clist_new, NULL);
	CHECK_INVALID(list, clist_new);

    RTMP_LIST_SEEK_TO_FIRST(list);
    list->prev = clist_new;
    clist_new->next = list;
    return clist_new;
}

RTMP_LIST_PTR  rtmp_list_pop_front ( RTMP_LIST_PTR list,  void **val )
{
    RTMP_LIST_PTR clist_del = NULL;

	CHECK_INVALID(list, NULL);

    RTMP_LIST_SEEK_TO_FIRST(list);

    if(NULL != val)
    {   
    	*val = list->val;
    }

    clist_del = list;
    list = list->next;
	if ( NULL != list)
	{
		list->prev = NULL;
	}
    rtmp_list_delete(clist_del, NULL);

    return list;
}
RTMP_LIST_PTR rtmp_list_push_back ( RTMP_LIST_PTR list,  void *val )
{
    RTMP_LIST_PTR clist_new;
	
	clist_new = rtmp_list_new ( val );

	CHECK_INVALID(clist_new, NULL);
	CHECK_INVALID(list, clist_new);

    RTMP_LIST_SEEK_TO_LAST ( list ) ;

    list->next = clist_new;
    clist_new->prev = list;

    return clist_new;
}

RTMP_LIST_PTR  rtmp_list_pop_back     ( RTMP_LIST_PTR list,  void **	val )
{
    RTMP_LIST_PTR clist_del = NULL;

	CHECK_INVALID(list, NULL);

    RTMP_LIST_SEEK_TO_LAST(list);

    if(NULL != val)
    {    
        *val = list->val;
    }
    clist_del = list;
    list = list->prev;
    if ( NULL != list)
    {
        list->next = NULL;
    }
    rtmp_list_delete(clist_del, NULL);

    return list;
}

BOOLEAN rtmp_list_append ( RTMP_LIST_PTR *head, RTMP_LIST_PTR tail )
{	
	RTMP_LIST_PTR list;
	
 	CHECK_INVALID(head, FALSE);
 	CHECK_INVALID(tail, FALSE);
	/*链表的头*/
	RTMP_LIST_SEEK_TO_FIRST(tail);
	list = *head;
	if( NULL == list)
	{
		*head = tail;
	}
	else
	{
		RTMP_LIST_SEEK_TO_LAST(list);
		list->next = tail ;
		tail->prev = list ;
	} 
    return TRUE;
}

BOOLEAN rtmp_list_append_ex ( RTMP_LIST_PTR *list, void *val )
{	RTMP_LIST_PTR clistNew;

	CHECK_INVALID(list, FALSE);
	clistNew = rtmp_list_new(val);
	CHECK_INVALID(clistNew, FALSE);
	return rtmp_list_append(list, clistNew);	
}

RTMP_LIST_PTR   rtmp_list_insert_before ( RTMP_LIST_PTR list, void *val )
{
    RTMP_LIST_PTR clistNew, clistPrev;

	if(NULL == list)
	{
		return rtmp_list_push_front ( list, val );
	}
    clistNew = rtmp_list_new(val);
	CHECK_INVALID(clistNew, NULL);

    clistPrev = list->prev;
    if( NULL != clistPrev)
    {
        clistPrev->next = clistNew;
        clistNew->prev = clistPrev;
    }
    clistNew->next = list;
    list->prev = clistNew;

    return clistNew;
}

RTMP_LIST_PTR rtmp_list_insert_after ( RTMP_LIST_PTR list, void *val )
{
    RTMP_LIST_PTR clistNew, clistNext;


	if(NULL == list)
	{
		return rtmp_list_push_back ( list, val );
	}

    clistNew = rtmp_list_new(val);
	CHECK_INVALID(clistNew, NULL);


    clistNext = list->next;
    if(NULL != clistNext)
    {
        clistNext->prev = clistNew;
        clistNew->next = clistNext;
    }
    clistNew->prev = list;
    list->next = clistNew;

    return clistNew;    
}

void* rtmp_list_replace ( RTMP_LIST_PTR list, void *val)
{
    void *data;

	CHECK_INVALID(list, NULL);
    data = list->val;
	list->val = val;
    return data;
}

RTMP_LIST_PTR rtmp_list_remove ( RTMP_LIST_PTR list, RTMP_OBJECT_FREE_FUN fun)
{
    RTMP_LIST_PTR prev, next;
	RTMP_LIST_PTR retval = list ;
		
    if(NULL == list)
    {
        // do nothing
    }
    else if( NULL == list->next  && NULL == list->prev)
    {    //the only node
        rtmp_list_delete(list, fun);
		retval = NULL;
    }
    else if( NULL == list->next &&  NULL != list->prev)
    {    //the last node
    	prev = list->prev;
        prev->next = NULL;
        rtmp_list_delete(list, fun);
		retval = prev;
    }
    else if( NULL != list->next && NULL == list->prev)
    {    //the first node
        next = list->next;
        next->prev = NULL;
        rtmp_list_delete(list, fun);
		retval = next;
    }
    else
    {
        prev = list->prev;
        next = list->next;
        prev->next = next;
        next->prev = prev;
        rtmp_list_delete(list, fun);
		retval = prev;
    }
    return retval;
}
void* rtmp_list_get_index(RTMP_LIST_PTR list, INT32S index)
{
	CHECK_INVALID(list, NULL);
    RTMP_LIST_SEEK_TO_FIRST(list);	
	while(NULL != list)
	{
		if( 0 == index )
		{
			return list->val;
		}
		index--;
	}
	return NULL;
}
void* rtmp_list_get_val(RTMP_LIST_PTR list, void* val, RTMP_OBJECT_COMPARE_FUN fun)
{
	CHECK_INVALID(list, NULL);
	CHECK_INVALID(fun, NULL);
    RTMP_LIST_SEEK_TO_FIRST(list);
	while(NULL != list)
	{
		if( 0 == (*fun)(list->val, val) )
		{
			return list->val;
		}
		list = list->next;
	}
	return NULL;
}

INT32S rtmp_list_remove_all ( RTMP_LIST_PTR list, RTMP_OBJECT_FREE_FUN fun)
{
    RTMP_LIST_PTR temp;

	CHECK_INVALID(list, -1);
    RTMP_LIST_SEEK_TO_FIRST(list);
    while(NULL != list)
    {
        temp = list;
        list = list->next;
        rtmp_list_delete(temp, fun);
    }
    return 0;
}
BOOLEAN rtmp_list_is_same ( RTMP_LIST_PTR dst,  RTMP_LIST_PTR src )
{
	CHECK_INVALID(dst, FALSE);
	CHECK_INVALID(src, FALSE);		
    RTMP_LIST_SEEK_TO_FIRST(dst);
    RTMP_LIST_SEEK_TO_FIRST(src);
	return ( src == dst );
}
BOOLEAN rtmp_list_is_exist(RTMP_LIST_PTR list, void* val, RTMP_OBJECT_COMPARE_FUN fun)
{	
	CHECK_INVALID(fun, FALSE);
	CHECK_INVALID(list, FALSE);
    RTMP_LIST_SEEK_TO_FIRST(list);
    while(NULL != list)
    {
		if(0 == (*fun)(list->val, val) )
		{
			return TRUE;
		}
        list = list->next;
    }
	return FALSE;
}
BOOLEAN rtmp_list_delete_by_val(RTMP_LIST_PTR *list, void* val, 
	RTMP_OBJECT_COMPARE_FUN compare_fun,
	RTMP_OBJECT_FREE_FUN free_fun)
{
	RTMP_LIST_PTR head,cur;

	CHECK_INVALID(compare_fun, FALSE);
	CHECK_INVALID(list, FALSE);
	head = *list;
	CHECK_INVALID(head , FALSE);
	cur = head;
	while( NULL != cur)
	{
		if( 0 == (*compare_fun)(cur->val, val) ) /*开始删除*/
		{
			if( cur == head )/*删除头节点*/
			{
				if( NULL != cur->next)
				{
					cur->next->prev =  NULL;
				}
				*list = cur->next;
			}
			else
			{
				cur->prev->next = cur->next;
				if( NULL != cur->next)
				{
					cur->next->prev = cur->prev;
				}
				
			}
			rtmp_list_delete(cur, free_fun);
			return TRUE;
		}
		cur = cur->next;
	}
	return FALSE;
}

RTMP_LIST_PTR rtmp_list_remove_by_val(RTMP_LIST_PTR *head, void* val, RTMP_OBJECT_COMPARE_FUN compare_fun)
{
	RTMP_LIST_PTR cur;
	
	CHECK_INVALID(compare_fun, NULL);
	CHECK_INVALID(head, NULL);
	cur = *head;
	while( NULL != cur)
	{
		if( 0 == compare_fun(cur->val,val) )
		{
			if( *head  == cur)  /*删除首节点*/
			{
				*head = cur->next;
				if( NULL != cur->next)
				{
					cur->next->prev = NULL;
				}
			}
			else
			{
				cur->prev->next = cur->next;
				if( NULL != cur->next)
				{
					cur->next->prev = cur->prev;
				}
			}
			cur->next = NULL;
			cur->prev = NULL;
			break;
		}
		cur = cur->next;
	}
	return cur;
}


