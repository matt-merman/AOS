#include "info.h"

memory_node *shift_buffer(int lenght, int offset, memory_node *node)
{

   int dim = lenght - offset;
   
   printk("-------------HERE1 (%d)-(%d)-------------\n", lenght, offset);
   
   char *remaning_buff = kmalloc(dim, GFP_KERNEL);

   printk("-------------HERE2 (%d)-(%d)-------------\n", lenght, offset);

   AUDIT printk("%s: ALLOCATED %d bytes\n", MODNAME, dim);
   if (remaning_buff == NULL)
   {
      printk("%s: unable to allocate memory\n", MODNAME);
      return NULL;
   }

   strncpy(remaning_buff, &node->buffer[offset], lenght);
   kfree(node->buffer);

   node->buffer = kmalloc(dim, GFP_KERNEL);
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

int read(object_state *the_object, const char *buff, loff_t *off, size_t len, session *session)
{

   int ret, residual_bytes = len, lenght_buffer = 0;
   memory_node *current_node, *last_node;

   wait_queue_head_t *wq;
   wq = &the_object->wq;

   // Try to acquire the mutex atomically.
   // Returns 1 if the mutex has been acquired successfully,
   // and 0 on contention.
   ret = mutex_trylock(&(the_object->operation_synchronizer));
   if (!ret)
   {

      printk("%s: unable to get lock now\n", MODNAME);
      if (session->blocking == BLOCKING)
      {

         ret = blocking(session->timeout, &the_object->operation_synchronizer, wq);
         if (ret == 0) return 0;
      }
      else
         return 0;
   }

   ret = 0;
   //lenght_buffer -= *off;

   current_node = the_object->head;
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

   // PHASE 2: REMOVING
   current_node = the_object->head;

   if (current_node->buffer == NULL)
   {
      //*off += len - ret;
      mutex_unlock(&(the_object->operation_synchronizer));
      wake_up(wq);
      return ret;
   }

   lenght_buffer = strlen(current_node->buffer);
   
   //lenght_buffer -= *off;
   
   if (len > lenght_buffer)
   {

      residual_bytes = len;

      while (!(residual_bytes < lenght_buffer))
      {

         residual_bytes -= lenght_buffer;
         AUDIT printk("%s: removing data '%s' from the flow on dev\n", MODNAME, current_node->buffer);

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

         //*off += len - ret;
         mutex_unlock(&(the_object->operation_synchronizer));
         wake_up(wq);
         return ret;
      }

      the_object->head = shift_buffer(lenght_buffer, residual_bytes, last_node);
      if (the_object->head == NULL)
      {

         mutex_unlock(&(the_object->operation_synchronizer));
         //*off += len - ret;
         wake_up(wq);
         return ret;
      }
   }
   else if (len == lenght_buffer)
   {

      kfree(current_node->buffer);
      current_node->buffer = NULL;

      if (current_node->next != NULL)
      {
         the_object->head = current_node->next;
         kfree(current_node);
      }
   }
   else
   {

      current_node = shift_buffer(lenght_buffer, len, current_node);
      if (current_node == NULL)
      {
         mutex_unlock(&(the_object->operation_synchronizer));
         //*off += len - ret;
         wake_up(wq);
         return ret;
      }
   }

   //*off += len - ret;
   mutex_unlock(&(the_object->operation_synchronizer));
   wake_up(wq);
   return ret;
}