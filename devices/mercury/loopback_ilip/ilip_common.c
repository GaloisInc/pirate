
/* ------------- MERCURY SYSTEMS INC IP COPYRIGHT HEADER  ------------------
 *
 * Copyright:
 *  Copyright (c) 1984-2020, Mercury Systems, Inc.,
 *  Andover MA.,and all third party embedded software sources
 *  All rights reserved under the Copyright law of US.
 *  and international treaties.
 *
 * ------------- MERCURY SYSTEMS INC IP COPYRIGHT HEADER  -------------------*/

/*
 * This material is based upon work supported by the Defense Advanced
 * Research Projects Agency (DARPA) under Contract No. HR011-19-C-0105.
 * Any opinions, findings and conclusions or recommendations expressed
 * in this material ar ethose of the author(s) and do not necessarily
 * reflect the views of the Defense Advanced Research Projects Agency
 * (DARPA).
 */

#include <linux/slab.h>

#include "ilip_common.h"

bool gaps_ilip_read_do_read_first_time[GAPS_ILIP_CHANNELS] = {false};

struct ilip_session_info gaps_ilip_sessions[GAPS_ILIP_NSESSIONS] = { {0} }; 

/** 
 * @brief Number of session we have created, DCD requires 7, simple demo does 
 *        not use this
 */ 
unsigned int gaps_ilip_session_count = 0;

unsigned int 
gaps_ilip_get_session_index
(
  unsigned int session_id
)
{
  unsigned int i;
  
  for (i=0; i<GAPS_ILIP_NSESSIONS; i++)
  {
    if (gaps_ilip_sessions[i].session == session_id)
    {
      /* session exists, return index in session array */
      return i;
    }
  }
  return GAPS_ILIP_NSESSIONS;
}

bool 
gaps_ilip_remove_session_index
(
  unsigned int session_id
)
{
  unsigned int i;
  
  for (i=0; i<gaps_ilip_session_count; i++)
  {
    if (gaps_ilip_sessions[i].session == session_id) 
    {
      /* session exists, return index in session array */
      gaps_ilip_sessions[i].session = 0xffffffff;
      gaps_ilip_sessions[i].level_src = 0xffffffff;
      gaps_ilip_sessions[i].level_dst = 0xffffffff;
      gaps_ilip_sessions[i].minor_src = 0xffffffff;
      gaps_ilip_sessions[i].minor_dst = 0xffffffff;
      gaps_ilip_session_count--;

      return true;
    }
  }
  return false;
}

bool 
gaps_ilip_save_session_id
(
  unsigned int session_id, 
  unsigned int verbosity
)
{
  unsigned int i;
  
  for (i=0; i<GAPS_ILIP_NSESSIONS; i++)
  {
    if (gaps_ilip_sessions[i].session == session_id)
    {
      /* session exists, return false as we do not have to create the device */
      if (verbosity >= 6)
      {
	printk( KERN_INFO "gaps_ilip_save_session_id( Session: %.8x ) found at [%2u]\n", session_id, i );
      }
      return false;
    }
  }
  for (i=0; i<GAPS_ILIP_NSESSIONS; i++)
  {
    if (gaps_ilip_sessions[i].session == 0xffffffff)
    {
      /* session empty, save session id and return true */
      gaps_ilip_sessions[i].session = session_id;
      gaps_ilip_session_count++;
      if (verbosity >= 6)
      {
	printk( KERN_INFO "gaps_ilip_save_session_id( Session: %.8x ) New [%2u] Count: %u\n", session_id, i, gaps_ilip_session_count );
      }
      return true;
    }
  }
  printk( KERN_WARNING "gaps_ilip_save_session_id( Session: %.8x ) No Slot Found, Count: %u\n", session_id, gaps_ilip_session_count );
  
  return false;
}

/**
 * @brief Compute the application level index from the minor driver number 
 *  
 * @details The level as a value is 1 based i.e. we talk about level 1 or level 2, but in the 
 * driver the level is used as an index so it is zero based.
 * 
 * @author mdesroch (2/7/20)
 * 
 * @param mn Driver minor number
 * 
 * @return unsigned int Level index for driver operations.
 */
#if 1
unsigned int
gaps_ilip_get_level_from_minor
(
  struct gaps_ilip_dev * dev,
  unsigned int mn
)
{
  unsigned int level = 0xffffffffu;

  switch (mn)
  {
    case 1:
      level = 0;
      break;
    case 2:
      level = 0;
      break;
    case 3:
      level = 1;
      break;
    case 4:
      level = 1;
      break;
    default:
      if (dev != NULL)
      {
        /* We save the level, but return the level index */
        if (dev->src_level == 1)
        {
          level = 0;
        }
        else if (dev->src_level == 2)
        {
          level = 1;
        }
      }
      break;
  }

  return level;
}

unsigned int 
gaps_ilip_get_channel_from_minor
(
  struct gaps_ilip_dev * dev, 
  unsigned int mn
)
{
  unsigned int channel = 0xffffffffu;
  
  switch (mn)
  {
    case 1:
      channel = 0;
      break;
    case 2:
      channel = 0;
      break;
    case 3:
      channel = 1;
      break;
    case 4:
      channel = 1;
      break;
    case 5:
      channel = 2;
      break;
    case 6:
      channel = 2;
      break;
    case 7:
      channel = 3;
      break;
    case 8:
      channel = 3;
      break;
    default:
      break;
  }
  return channel;
}
#endif

#if 0
struct ilip_session_info * 
gaps_ilip_get_session
(
  struct gaps_ilip_dev * gaps_dev, 
  unsigned int session_id
)
{
  struct ilip_node * node;

  /* CHeck that gaps_dev pointer is valid */
  if (gaps_dev == NULL)
  {
    return NULL;
  }

  /* Search for the session with the matching session ID */
  node = gaps_ilip_list_match_fn(&gaps_dev->sessions,
                                 &gaps_dev->sessions_lock,
                                 gaps_ilip_session_match_fn,
                                 (void *)(u64)(session_id));
  if (node == NULL)
  {
    return NULL;
  }
  else
  {
    return (struct ilip_session_info *)(node->data);
  }
}

bool 
gaps_ilip_remove_session_index
(
  struct gaps_ilip_dev * gaps_dev, 
  unsigned int session_id
)
{
  struct ilip_node * node;

  if (gaps_dev == NULL)
  {
    return false;
  }

  node = gaps_ilip_list_match_fn(&gaps_dev->sessions,
                                 &gaps_dev->sessions_lock,
                                 gaps_ilip_session_match_fn,
                                 (void *)(u64)(session_id));
  if (node != NULL)
  {
    /* Free the session structure */
    kfree(node->data);

    /* Delete the session node */
    gaps_ilip_list_del(&gaps_dev->sessions, &gaps_dev->sessions_lock. node);

    /* Decrement the number of active sessions */
    gaps_ilip_session_count--;

    return true;
  }

  return false;
}

int 
gaps_ilip_save_session_id
(
  struct gaps_ilip_dev * gaps_dev, 
  unsigned int session_id
)
{
  struct ilip_session_info * session;
  struct ilip_node * node;
  int rv;

  if (gaps_dev == NULL)
  {
    return -EINVAL;
  }

  /* Check if a session with this ID already exists */
  node = gaps_ilip_list_match_fn(&gaps_dev->sessions,
                                 &gaps_dev->sessions_lock,
                                 gaps_ilip_session_match_fn,
                                 (void *)(u64)(session_id));
  if (node != NULL)
  {
    /* Entry with session ID exists */
    return -EEXIST;
  }

  /* Allocate a new session structure */
  session = kzalloc(sizeof(struct ilip_session_info), GFP_KERNEL);
  if (session == NULL)
  {
    return -ENOMEM;
  }

  /* Set the session ID for the new session */
  session->session = session_id;

  /* Add the session to the list of sessions */
  rv = gaps_ilip_list_add(&gaps_dev->sessions,
                          &gaps_dev->sessions_lock,
                          session);
  return rv;
}

bool 
gaps_ilip_session_match_fn
(
  void * node, 
  void * arg
)
{
  struct ilip_session_info * session;
  unsigned int session_id;

  /* Convert arguments to expected types */
  session = (struct ilip_session_info *)(node);
  session_id = (unsigned int)(u64)(arg);

  /* See if we have a match */
  if (session->session == session_id)
  {
    return true;
  }

  return false;
}

bool 
gaps_ilip_minor_match_fn
(
  void * node, 
  void * arg
)
{
  struct gaps_ilip_dev * dev;
  unsigned int mn;

  /* Convert arguments to expected types */
  dev = (struct gaps_ilip_dev *)(node);
  mn = (unsigned int)(u64)(arg);

  /* See if we have a match */
  if (dev->mn == mn)
  {
    return true;
  }

  return false;
}
#endif

bool 
gaps_ilip_smb_match_fn
(
  void * node, 
  void * arg
)
{
  smb_t * smb;
  dma_addr_t handle;

  /* Convert arguments to expected types */
  smb = (smb_t *)(node);
  handle = (dma_addr_t)(arg);

  /* See if we have a match */
  if ((smb->handle == handle) && (smb->completion_rcvd == 0))
  {
    return true;
  }

  return false;
}

struct ilip_node * 
gaps_ilip_list_find
(
  struct ilip_node ** head, 
  spinlock_t * lock, 
  void * data
)
{
  struct ilip_node * node;
  unsigned long flags;

  /* Check list is valid */
  if (head == NULL)
  {
    return NULL;
  }

  /* If lock is valid, lock the list */
  if (lock)
  {
    spin_lock_irqsave(lock, flags);
  }

  /* Start at the head of the list */
  node = *head;

  /* Walk the list */
  while (node != NULL)
  {
    /* See if list member matches */
    if (node->data == data)
    {
      /* Unlock the list, if locked */
      if (lock)
      {
        spin_unlock_irqrestore(lock, flags);
      }

      /* We have a match, return the node */
      return node;
    }

    /* Advance to the next node in the list */
    node = node->next;
  }

  /* Unlock the list, if locked */
  if (lock)
  {
    spin_unlock_irqrestore(lock, flags);
  }

  /* No match found, return a null pointer */
  return NULL;
}

void * 
gaps_ilip_list_match_fn
(
  struct ilip_node ** head, 
  spinlock_t * lock, 
  bool (*fn)(void *, void *), 
  void * arg
)
{
  struct ilip_node * node;
  unsigned long flags;

  /* Check list is valid */
  if (head == NULL)
  {
    return NULL;
  }


  /* If lock is valid, lock the list */
  if (lock)
  {
    spin_lock_irqsave(lock, flags);
  }

  /* Get the head of the list */
  node = *head;

  /* Walk the list */
  while (node != NULL)
  {
    /* Does the node pass the matching function ? */
    if (fn(node->data, arg) == true)
    {
      /* Unlock the list, if necessary */
      if (lock)
      {
        spin_unlock_irqrestore(lock, flags);
      }

      /* Return the matching data pointer */
      return node->data;
    }

    /* Advance to the next node in the list */
    node = node->next;
  }

  /* Unlock the list, if necessary */
  if (lock)
  {
    spin_unlock_irqrestore(lock, flags);
  }

  /* No match found, return a null pointer */
  return NULL;
}

int 
gaps_ilip_list_add
(
  struct ilip_node ** head, 
  spinlock_t * lock, 
  void * data
)
{
  struct ilip_node * node;
  struct ilip_node * last;

  unsigned long flags;

  /* Allocate a new list entry */
  node = (struct ilip_node * )kzalloc(sizeof(struct ilip_node), GFP_KERNEL);
  if (node == NULL)
  {
    return -ENOMEM;
  }

  /* Assign the data to the node */
  node->data = data;

  /* Set the next pointer of the new node to NULL */
  node->next = NULL;

  /* Lock the list */
  spin_lock_irqsave(lock, flags);

  /* Save the current head of the list */
  last = *head;

  /* Check if the list is empty */
  if (*head == NULL)
  {
    /* List is empty so no previous node */
    node->prev = NULL;

    /* Insert the node as the list head */
    *head = node;

    /* Release the list lock */
    spin_unlock_irqrestore(lock, flags);

    return 0;
  }

  /* Walk to the end of the list */
  while (last->next != NULL)
  {
    last = last->next;
  }

  /* Insert the new node at the end of the list */
  last->next = node;

  /* Set the previous link for the new node */
  node->prev = last;

  /* Release the list lock */
  spin_unlock_irqrestore(lock, flags);

  return 0;
}

void 
gaps_ilip_list_del
(
  struct ilip_node ** head, 
  spinlock_t * lock, 
  void * data
)
{
  struct ilip_node * node;
  unsigned long flags;

  /* Lock the list */
  spin_lock_irqsave(lock, flags);

  /* Search the list for the matching node */
  node = gaps_ilip_list_find(head, NULL, data);
  if (node != NULL)
  {
    /* Is the node the head node ? */
    if (node == *head)
    {
      /* Move the head to the next member */
      *head = node->next;
    }

    /* Does the node have a next member ? */
    if (node->next != NULL)
    {
      /* Adjust the previous pointer for the next member */
      node->next->prev = node->prev;
    }

    /* Does the node have a previous member ? */
    if (node->prev != NULL)
    {
      /* Adjust the next pointer for the previous member */
      node->prev->next = node->next;
    }

    /* Free the node being deleted */
    kfree(node);
  }

  /* Release the list lock */
  spin_unlock_irqrestore(lock, flags);
}

uint 
gaps_ilip_get_nt_verbose_level
(
  void
)
{
  return gaps_ilip_nt_verbose_level;
}

unsigned int 
jenkins_one_at_a_time_hash_done
(
  unsigned int hash
)
{
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash;
}

unsigned int 
jenkins_one_at_a_time_hash_ex
(
  unsigned int hash, 
  const unsigned char * key, 
  size_t len
)
{
  uint32_t i;

  for(i = 0; i < len; ++i) 
  {
    hash += key[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }

  return hash;
}

unsigned int 
jenkins_one_at_a_time_hash_init
(
  size_t len, 
  unsigned int initval
)
{
  uint32_t hash;

  hash = 0xdeadbeef + ((unsigned int)len) + initval;

  return hash;
}

unsigned int 
jenkins_one_at_a_time_hash
(
  const void * key, 
  size_t len, 
  unsigned int initval
)
{
  uint32_t hash;

  hash = 0xdeadbeef + ((unsigned int)len) + initval;
  hash = jenkins_one_at_a_time_hash_ex(hash, key, len);
  hash = jenkins_one_at_a_time_hash_done(hash);

  return hash;
}

