/*
Copyright 2011 Aiko Barz

This file is part of torrentkino.

torrentkino is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

torrentkino is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with torrentkino.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <sys/epoll.h>

#include "cache.h"
#include "torrentkino.h"
#include "p2p.h"
#include "hex.h"

struct obj_cache *cache_init( void ) {
	struct obj_cache *cache = (struct obj_cache *) myalloc( sizeof(struct obj_cache) );
	cache->list = list_init();
	cache->hash = hash_init( CACHE_SIZE );
	return cache;
}

void cache_free( void ) {
	list_clear( _main->cache->list );
	list_free( _main->cache->list );
	hash_free( _main->cache->hash );
	myfree( _main->cache );
}

void cache_put( UCHAR *target, UCHAR *nodes_compact_list, int nodes_compact_size ) {
	ITEM *item = NULL;
	CACHE *cache = cache_find( target );

	/* Found in cache, update cache, done */
	if( cache != NULL ) {
		cache_update( cache, target, nodes_compact_list, nodes_compact_size );
		return;
	}

	cache = (CACHE *) myalloc( sizeof(CACHE) );
	cache_update( cache, target, nodes_compact_list, nodes_compact_size );
	
	time_add_30_min( &cache->lifetime );
	time_add_5_min_approx( &cache->renew );

	item = list_put( _main->cache->list, cache );
	hash_put( _main->cache->hash, cache->target, SHA1_SIZE, item );
}

void cache_del( ITEM *item ) {
	CACHE *cache = list_value( item );
	hash_del( _main->cache->hash, (UCHAR *)cache->target, SHA1_SIZE );
	list_del( _main->cache->list, item );
	myfree( cache );
}

void cache_expire( time_t now ) {
	ITEM *item = NULL;
	ITEM *next = NULL;
	CACHE *cache = NULL;

	item = list_start( _main->cache->list );
	while( item != NULL ) {
		next = list_next( item );

		/* 30 minutes without activity. Kill it. */
		cache = list_value( item );
		if( now > cache->lifetime ) {
			cache_del( item );
		}

		item = next;
	}
}

void cache_renew( time_t now ) {
	ITEM *item = NULL;
	CACHE *cache = NULL;

	item = list_start( _main->cache->list );
	while( item != NULL ) {

		/* Lookup target on my own every 5 minutes */
		cache = list_value( item );
		if( now > cache->renew ) {
			p2p_localhost_lookup_remote( cache->target, NULL );
			time_add_5_min_approx( &cache->renew );
		}

		item = list_next( item );
	}
}

void cache_update( CACHE *cache, UCHAR *target, UCHAR *nodes_compact_list, int nodes_compact_size ) {
	memcpy(cache->target, target, SHA1_SIZE );
	memcpy(cache->nodes_compact_list, nodes_compact_list, nodes_compact_size );
	cache->nodes_compact_size = nodes_compact_size;
}

CACHE *cache_find( UCHAR *target ) {
	ITEM *item = NULL;

	if( ( item = hash_get( _main->cache->hash, target, SHA1_SIZE ) ) == NULL ) {
		return NULL;
	}

	return list_value( item );
}

int cache_compact_list( UCHAR *nodes_compact_list, UCHAR *target ) {
	CACHE *cache = cache_find( target );

	/* Not found */
	if( cache == NULL ) {
		return 0;
	}

	/* Success */
	memcpy( nodes_compact_list, cache->nodes_compact_list,
		cache->nodes_compact_size );
	
	/* Extend valid lifetime */
	time_add_30_min( &cache->lifetime );

	return cache->nodes_compact_size;
}
