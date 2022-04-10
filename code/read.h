#include "common.h"

memory_node *shift_buffer(int, int, memory_node *);
int read(object_state *, char *, loff_t *, size_t, session *, int);

memory_node *shift_buffer(int lenght, int offset, memory_node *node)
{

   int dim = lenght - offset;

   char *remaning_buff = kzalloc(dim, GFP_ATOMIC);

   AUDIT printk("%s: ALLOCATED %d bytes\n", MODNAME, dim);
   if (remaning_buff == NULL)
   {
      printk("%s: unable to allocate memory\n", MODNAME);
      return NULL;
   }

   strncpy(remaning_buff, &node->buffer[offset], lenght);
   kfree(node->buffer);

   node->buffer = kzalloc(dim, GFP_ATOMIC);
   AUDIT printk("%s: ALLOCATED %d bytes\n", MODNAME, dim);
   if (node->buffer == NULL)
   {
      printk("%s: unable to allocate memory\n", MODNAME);
      return NULL;
   }

   strncpy(node->buffer, remaning_buff, dim);
   kfree(remaning_buff);

   return node;
}

int read(object_state *the_object,
         char *buff,
         loff_t *off,
         size_t len,
         session *session,
         int minor)
{

   int ret = 0, residual_bytes = len, lenght_buffer = 0, exit_mem = 0;
   memory_node *current_node, *last_node;
   wait_queue_head_t *wq;

   wq = get_lock(the_object, session, minor);
   if (wq == NULL)
      return -EAGAIN;

   *off = 0;

   current_node = the_object->head;
   if (current_node->buffer == NULL)
      goto exit;

   // PHASE 1: READING
   while (residual_bytes != 0)
   {

      if (current_node->buffer == NULL)
         break;

      lenght_buffer = strlen(current_node->buffer);

      if (lenght_buffer < residual_bytes)
      {

         residual_bytes -= lenght_buffer;
         ret += copy_to_user(&buff[ret], &current_node->buffer[*off], lenght_buffer);
         ret += lenght_buffer;

         if (current_node->next == NULL)
            break;
         else
            current_node = current_node->next;
      }
      else
      {

         ret += copy_to_user(&buff[ret], &current_node->buffer[*off], residual_bytes);
         ret += residual_bytes;
         current_node = current_node->next;
         break;
      }
   }

   current_node = the_object->head;
   lenght_buffer = strlen(current_node->buffer);
   residual_bytes = len;

   // PHASE 2: REMOVING
   while (!(residual_bytes < lenght_buffer))
   {

      residual_bytes -= lenght_buffer;
      AUDIT printk("%s: removing data '%s' from the flow on dev\n", MODNAME, current_node->buffer);

      if (residual_bytes == 0)
      {

         kfree(current_node->buffer);
         current_node->buffer = NULL;

         if (current_node->next != NULL)
         {
            the_object->head = current_node->next;
            kfree(current_node);
         }

         goto exit;
      }

      last_node = current_node->next;
      kfree(current_node->buffer);
      kfree(current_node);

      if (last_node != NULL && last_node->buffer != NULL)
      {
         lenght_buffer = strlen(last_node->buffer);
         current_node = last_node;
         continue;
      }

      else if (last_node->buffer == NULL)
         the_object->head = last_node;

      goto exit;
   }

   if (len > lenght_buffer)
   {
      the_object->head = shift_buffer(lenght_buffer, residual_bytes, last_node);
      if (the_object->head == NULL)
         exit_mem = 1;
   }
   else
   {

      current_node = shift_buffer(lenght_buffer, len, current_node);
      if (current_node == NULL)
         exit_mem = 1;
   }

exit:

   *off += len - ret;

   if (session->priority == HIGH_PRIORITY)
      hp_bytes[minor] -= ret;
   else
      lp_bytes[minor] -= ret;

   mutex_unlock(&(the_object->operation_synchronizer));
   wake_up(wq);

   if (exit_mem)
      return -ENOMEM;

   return ret;
}