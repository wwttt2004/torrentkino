/*
Copyright 2006 Aiko Barz

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
#include <string.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>

#ifdef TUMBLEWEED
#include "tumbleweed.h"
#include "str.h"
#include "malloc.h"
#include "list.h"
#include "node_tcp.h"
#include "log.h"
#include "file.h"
#include "unix.h"
#endif

#ifdef TORRENTKINO
#include "torrentkino.h"
#include "log.h"
#endif

#include "conf.h"

struct obj_conf *conf_init( int argc, char **argv ) {
	struct obj_conf *conf = (struct obj_conf *) myalloc( sizeof(struct obj_conf) );
	BEN *opts = opts_init();
	BEN *value = NULL;

	/* Parse command line */
	opts_load( opts, argc, argv );

	/* Mode */
	if( ben_searchDictStr( opts, "-d" ) != NULL ) {
		conf->mode = CONF_DAEMON;
	} else {
		conf->mode = CONF_CONSOLE;
	}

	/* Verbosity */
	if( ben_searchDictStr( opts, "-v" ) != NULL ) {
		conf->verbosity = CONF_VERBOSE;
	} else if ( ben_searchDictStr( opts, "-q" ) != NULL ) {
		conf->verbosity = CONF_BEQUIET;
	} else {
		/* Be verbose in the console and quiet while running as a daemon. */
		conf->verbosity = ( conf->mode == CONF_CONSOLE ) ?
			CONF_VERBOSE : CONF_BEQUIET;
	}

	/* Port */
	value = ben_searchDictStr( opts, "-p" );
	if( value != NULL && ben_str_size( value ) >= 1 ) {
		conf->port = atoi( (char *)value->v.s->s );
	} else {
		conf->port = CONF_PORT;
	}
	if( conf->port < CONF_PORTMIN || conf->port > CONF_PORTMAX ) {
		conf->port = CONF_PORT;
	}

	/* Cores */
	conf->cores = ( unix_cpus() > 2 ) ? unix_cpus() : CONF_CORES;
	if( conf->cores < 1 || conf->cores > 128 ) {
		fail( "Invalid number of CPU cores" );
	}

	/* HOME */
	conf_home( conf, opts );

#ifdef TUMBLEWEED
	/* HTML index */
	value = ben_searchDictStr( opts, "-i" );
	if( value != NULL && ben_str_size( value ) >= 1 ) {
		snprintf( conf->file, BUF_SIZE, "%s", (char *)value->v.s->s );
	} else {
		snprintf( conf->file, BUF_SIZE, "%s", CONF_INDEX_NAME );
	}
	if( !str_isValidFilename( conf->file ) ) {
		fail( "Index %s looks suspicious", conf->file );
	}
#endif

#ifdef TORRENTKINO
	/* Hostname */
	conf_hostname( conf, opts );

	/* Realm */
	value = ben_searchDictStr( opts, "-r" );
	if( value != NULL && ben_str_size( value ) >= 1 ) {
		snprintf( conf->realm, BUF_SIZE, "%s", (char *)value->v.s->s );
		conf->bool_realm = TRUE;
	} else {
		snprintf( conf->realm, BUF_SIZE, "%s", CONF_REALM );
		conf->bool_realm = FALSE;
	}

	/* Compute host_id. Respect the realm. */
	conf_hostid( conf->host_id, conf->hostname,
		conf->realm, conf->bool_realm );

	/* Announce this port */
	value = ben_searchDictStr( opts, "-b" );
	if( value != NULL && ben_str_size( value ) >= 1 ) {
		conf->announce_port = atoi( (char *)value->v.s->s );
	} else {
		conf->announce_port = CONF_ANNOUNCED_PORT;
	}
	if( conf->announce_port < CONF_PORTMIN || conf->announce_port > CONF_PORTMAX ) {
		fail( "Invalid announce port number. (-a)" );
	}

	if( getuid() == 0 ) {
		snprintf( conf->file, BUF_SIZE, "%s/%s", conf->home, CONF_FILE );
	} else {
		snprintf( conf->file, BUF_SIZE, "%s/.%s", conf->home, CONF_FILE );
	}

	/* Node ID */
	value = ben_searchDictStr( opts, "-n" );
	if( value != NULL && ben_str_size( value ) >= 1 ) {
		sha1_hash( conf->node_id, (char *)value->v.s->s, ben_str_size( value ) );
	} else {
		rand_urandom( conf->node_id, SHA1_SIZE );
	}

	memset( conf->null_id, '\0', SHA1_SIZE );

	/* Bootstrap node */
	value = ben_searchDictStr( opts, "-x" );
	if( value != NULL && ben_str_size( value ) >= 1 ) {
		snprintf( conf->bootstrap_node, BUF_SIZE, "%s", (char *)value->v.s->s );
	} else {
		snprintf( conf->bootstrap_node, BUF_SIZE, "%s", CONF_MULTICAST );
	}

	/* Bootstrap port */
	value = ben_searchDictStr( opts, "-y" );
	if( value != NULL && ben_str_size( value ) >= 1 ) {
		snprintf( conf->bootstrap_port, CONF_PORT_SIZE+1, "%s",
			value->v.s->s );
	} else {
		snprintf( conf->bootstrap_port, CONF_PORT_SIZE+1, "%i", CONF_PORT );
	}
	if( str_isSafePort( conf->bootstrap_port) < 0 ) {
		fail( "Invalid bootstrap port number. (-y)" );
	}

#ifdef POLARSSL
	/* Secret key */
	value = ben_searchDictStr( opts, "-k" );
	if( value != NULL && ben_str_size( value ) >= 1 ) {
		snprintf( conf->key, BUF_SIZE, "%s", (char *)value->v.s->s );
		conf->bool_encryption = TRUE;
	} else {
		memset( conf->key, '\0', BUF_SIZE );
		conf->bool_encryption = FALSE;
	}
#endif
#endif

	opts_free( opts );

	return conf;
}

void conf_free( void ) {
	myfree( _main->conf );
}

void conf_home( struct obj_conf *conf, BEN *opts ) {
#ifdef TUMBLEWEED
	BEN *value = NULL;
#endif

#ifdef TORRENTKINO
	if( getenv( "HOME" ) == NULL || getuid() == 0 ) {
		strncpy( conf->home, "/etc", BUF_OFF1 );
	} else {
		snprintf( conf->home,  BUF_SIZE, "%s", getenv( "HOME") );
	}
#endif

#ifdef TUMBLEWEED
	value = ben_searchDictStr( opts, "-w" );
	if( value != NULL && ben_str_size( value ) >= 1 ) {
		char *p = NULL;

		p = (char *)value->v.s->s;;

		/* Absolute path or relative path */
		if( *p == '/' ) {
			snprintf( conf->home, BUF_SIZE, "%s", p );
		} else if ( getenv( "PWD" ) != NULL ) {
			snprintf( conf->home, BUF_SIZE, "%s/%s", getenv( "PWD" ), p );
		} else {
			strncpy( conf->home, "/var/www", BUF_OFF1 );
		}
	} else {
		if( getenv( "HOME" ) == NULL || getuid() == 0 ) {
			strncpy( conf->home, "/var/www", BUF_OFF1 );
		} else {
			snprintf( conf->home, BUF_SIZE, "%s/%s", getenv( "HOME"), "Public" );
		}
	}
#endif

	if( !file_isdir( conf->home ) ) {
		fail( "%s does not exist", conf->home );
	}
}

#ifdef TORRENTKINO
void conf_hostname( struct obj_conf *conf, BEN *opts ) {
	BEN *value = NULL;
	char *f = NULL;
	char *p = NULL;
	
	/* Hostname from args */
	value = ben_searchDictStr( opts, "-a" );
	if( value != NULL && ben_str_size( value ) >= 1 ) {
		snprintf( conf->hostname, BUF_SIZE, "%s", (char *)value->v.s->s );
		return;
	} else {
		strncpy( conf->hostname, "bulk.p2p", BUF_OFF1 );
	}

	/* Hostname from file */
	if( ! file_isreg( CONF_HOSTFILE ) ) {
		return;
	}

	f = (char *) file_load( CONF_HOSTFILE, 0, file_size( CONF_HOSTFILE ) );

	if( f == NULL ) {
		return;
	}
		
	if( ( p = strchr( f, '\n')) != NULL ) {
		*p = '\0';
	}

	snprintf( conf->hostname, BUF_SIZE, "%s.p2p", f );

	myfree( f );
}

void conf_hostid( UCHAR *host_id, char *hostname, char *realm, int bool ) {
	UCHAR sha1_buf1[SHA1_SIZE];
	UCHAR sha1_buf2[SHA1_SIZE];
	int j = 0;

	/* The realm influences the way, the lookup hash gets computed */
	if( bool == TRUE ) {
		sha1_hash( sha1_buf1, hostname, strlen( hostname ) );
		sha1_hash( sha1_buf2, realm, strlen( realm ) );

		for( j = 0; j < SHA1_SIZE; j++ ) {
			host_id[j] = sha1_buf1[j] ^ sha1_buf2[j];
		}
	} else {
		sha1_hash( host_id, hostname, strlen( hostname ) );
	}
}
#endif

void conf_print( void ) {
#ifdef TORRENTKINO
	char hex[HEX_LEN];
#endif

	info( NULL, 0, "Threads: %i", _main->conf->cores );
	
	if( _main->conf->mode == CONF_CONSOLE ) {
		info( NULL, 0, "Mode: Console (-d)" );
	} else {
		info( NULL, 0, "Mode: Daemon (-d)" );
	}

	if( _main->conf->verbosity == CONF_BEQUIET ) {
		info( NULL, 0, "Verbosity: Quiet (-q/-v)" );
	} else {
		info( NULL, 0, "Verbosity: Verbose (-q/-v)" );
	}

#ifdef TUMBLEWEED
	info( NULL, 0, "Workdir: %s (-w)", _main->conf->home );
#elif TORRENTKINO
	info( NULL, 0, "Workdir: %s", _main->conf->home );
#endif

#ifdef TUMBLEWEED
	info( NULL, 0, "Index file: %s (-i)", _main->conf->file );
#elif TORRENTKINO
	info( NULL, 0, "Config file: %s", _main->conf->file );
#endif

#ifdef TUMBLEWEED
	info( NULL, 0, "Listen to TCP/%i (-p)", _main->conf->port );
#elif TORRENTKINO
	info( NULL, 0, "Listen to UDP/%i (-p)", _main->conf->port );
#endif

#ifdef TORRENTKINO
	info( NULL, 0, "Hostname: %s (-a)", _main->conf->hostname );

	hex_hash_encode( hex, _main->conf->node_id );
	info( NULL, 0, "Node ID: %s", hex );

	hex_hash_encode( hex, _main->conf->host_id );
	info( NULL, 0, "Host ID: %s", hex );

	info( NULL, 0, "Bootstrap Node: %s (-x)", _main->conf->bootstrap_node );
	info( NULL, 0, "Bootstrap Port: UDP/%s (-y)", _main->conf->bootstrap_port );
	info( NULL, 0, "Announce Port: %i (-a)", _main->conf->announce_port );

	/* Realm */
	if( _main->conf->bool_realm == 1 ) {
		info( NULL, 0, "Realm: %s (-r)", _main->conf->realm );
	} else {
		info( NULL, 0, "Realm: None (-r)" );
	}

	/* Encryption */
#ifdef POLARSSL
	if( _main->conf->bool_encryption == 1 ) {
		info( NULL, 0, "Encryption key: %s (-k)", _main->conf->key );
	} else {
		info( NULL, 0, "Encryption key: None (-k)" );
	}
#endif
#endif
}

#ifdef TORRENTKINO
void conf_write( void ) {
	BEN *dict = ben_init( BEN_DICT );
	BEN *key = NULL;
	BEN *val = NULL;
	RAW *raw = NULL;

	/* Port */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_INT );
	ben_str( key, (UCHAR *)"port", 4 );
	ben_int( val, _main->conf->port );
	ben_dict( dict, key, val );

	/* IP mode */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_INT );
	ben_str( key, (UCHAR *)"ip_version", 10 );
#ifdef IPV6
	ben_int( val, 6 );
#elif IPV4
	ben_int( val, 4 );
#endif
	ben_dict( dict, key, val );

	raw = ben_enc( dict );

	if( !file_write( _main->conf->file, (char *)raw->code, raw->size ) ) {
		fail( "Writing %s failed", _main->conf->file );
	}

	raw_free( raw );
	ben_free( dict );
}
#endif

int conf_verbosity( void ) {
	return _main->conf->verbosity;
}

int conf_mode( void ) {
	return _main->conf->mode;
}
