 /*
  * io_util.h
  * 
  * Copyright 2013 qinbh <buaaqbh@gmail.com>
  * 
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  * 
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  * 
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
  * 02110-1301  USA.
  * 
  */
 
 #ifndef IO_UTIL_H
 #define IO_UTIL_H
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 size_t io_readn(int fd, void *buf, size_t nbytes, unsigned int timout);
 size_t io_writen(int fd, const void *ptr, size_t n);
 
 #ifdef __cplusplus
 } /* extern "C" */
 #endif
 #endif /* IO_UTIL_H */
