/*BHEADER**********************************************************************
 * (c) 1995   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision: 1.1.1.1 $
 *********************************************************************EHEADER*/
#include "amps.h"

#define AMPS_COPY(type, src, len, src_stride, dest, dest_stride) \
{ \
     type *ptr_src, *ptr_dest; \
     if( ((src_stride) == 1) && ((dest_stride == 1))) \
	memcpy((dest), (src), (len)*sizeof(type)); \
     else \
	for(ptr_src = (type*)(src), (ptr_dest) = (type*)(dest); \
	    (ptr_dest) < (type*)(dest) + (len)*(dest_stride); \
	    (ptr_src) += (src_stride), (ptr_dest) += (dest_stride)) \
		    *(ptr_dest) = *(ptr_src); \
} 


void amps_vector_copy(type, items, dim, ptr_src, len, ptr_dst, dst_stride)
int type;
amps_PackageItem *items;
int dim;
char **ptr_src;
int *len;
char **ptr_dst;
int *dst_stride;
{
   int i;
   int src_stride;

   src_stride = items[dim].stride;   

   if(dim == 0)
   {
      if(type == AMPS_INVOICE_DOUBLE_CTYPE)
      {
	 if(((src_stride) == 1) && ((*dst_stride == 1)))
	 {
	    memcpy(*ptr_dst, *ptr_src, (*len)*sizeof(double));
	    *(double **)ptr_src += (*len)-1;
	    *(double **)ptr_dst += (*len)-1;
	 }
	 else 
	 {
	    for(i = 0; i < *len-1; i++)
	    {
	       **((double **)ptr_dst) = **((double **)ptr_src);
	       *(double **)ptr_src += src_stride;
	       *(double **)ptr_dst += *dst_stride;
	    }
	    **((double **)ptr_dst) = **((double **)ptr_src);
	 } 
      }
      else
	 printf("AMPS Error: invalid vector type\n");
   }
   else
   {
      for(i = 0; i < len[dim]-1; i++)
      {
	 amps_vector_copy(type, items, dim-1, ptr_src, 
			  len, ptr_dst, dst_stride);

	 *(double **)ptr_src += src_stride;
	 *(double **)ptr_dst += dst_stride[dim];
      }

      amps_vector_copy(type, items, dim-1, ptr_src, len, ptr_dst, dst_stride);
   }
}

void _amps_wait_exchange(amps_Handle handle)
{
   amps_Package package = handle -> package;
   amps_InvoiceEntry *ptr;   
   amps_PackageItem *items;

   char *src;
   int src_len, src_stride;
   char *dst;
   int dst_stride;

   int i, j;
   int item;

   /* exchange here so things have greater likely good of being in cache */
   for(i = 0; i < package -> num_recv; i++)
   {

#if 1
      /* first make sender has signaled a copy */
      uspsema(package -> rcv_info[i] -> send_sema);
#endif

      items = package -> rcv_info[i] -> items;
      ptr = package -> recv_invoices[i] -> list;      

      item = 0;
      /* For each of the src copy the data the package information */
      for(j = 0; j < package -> recv_invoices[i] -> num; j++, ptr = ptr -> next)
      {
	 if(items[item].type > AMPS_INVOICE_LAST_CTYPE )
	 {

	    src = items[item].data;
	    dst = ptr -> data;

	    amps_vector_copy(items[item].type-AMPS_INVOICE_LAST_CTYPE, 
			     &items[item], 
			     items[item].dim-1, 
			     &src, 
			     ptr -> ptr_len, 
			     &dst,
			     ptr -> ptr_stride);
	    item += items[item].dim;
	 }
	 else
	 {
	    src = items[item].data;
	    
	    src_len = items[item].len;
	    src_stride = items[item].stride;
	    
	    dst = ptr -> data;
	    
	    dst_stride = (ptr -> stride_type == AMPS_INVOICE_POINTER) ? 
	       *(ptr -> ptr_stride) : ptr -> stride;
	    
	    switch(items[item].type)
	    {
	    case AMPS_INVOICE_CHAR_CTYPE:
	       AMPS_COPY(char, src, src_len, src_stride, dst, dst_stride);
	       break;
	    case AMPS_INVOICE_SHORT_CTYPE:
	       AMPS_COPY(short, src, src_len, src_stride, dst, dst_stride);
	       break;
	    case AMPS_INVOICE_INT_CTYPE:
	       AMPS_COPY(int, src, src_len, src_stride, dst, dst_stride);
	       break;
	    case AMPS_INVOICE_LONG_CTYPE:
	       AMPS_COPY(long, src, src_len, src_stride, dst, dst_stride);
	       break;
	    case AMPS_INVOICE_FLOAT_CTYPE:
	       AMPS_COPY(float, src, src_len, src_stride, dst, dst_stride);
	       break;
	    case AMPS_INVOICE_DOUBLE_CTYPE:
	       AMPS_COPY(double, src, src_len, src_stride, dst, dst_stride);
	       break;
	    }

	    item++;
	 }
      }
#if 1      
      /* signal to sender that we have copied the data */
      usvsema(package -> rcv_info[i] -> recv_sema);
#endif
   }
    
#if 0
   /* Need to sync here so we know everyone is done */
   barrier(amps_shmem_info -> sync, amps_size);
#else

   i= package -> num_send;
   while(i--)
   {
      /* make sure that reciever has copied date before we continue */
      uspsema(package -> snd_info[i] -> recv_sema);
   }
#endif

}

amps_Handle amps_IExchangePackage(amps_Package package)
{

#if 0
   barrier(amps_shmem_info -> sync, amps_size);   
#else
   int i;

   i= package -> num_send;
   while(i--)
   {
      /* we are ready for a copy to occur, signal it */
      usvsema(package -> snd_info[i] -> send_sema);
   }
#endif
   return( amps_NewHandle(NULL, 0, NULL, package));
}

