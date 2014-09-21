
/* MODULE							HTAABrow.c
**		BROWSER SIDE ACCESS AUTHORIZATION MODULE
**
**	Containts the code for keeping track on server hostnames,
**	port numbers, scheme names, usernames, passwords
**	(and servers' public keys).
**
** IMPORTANT:
**	Routines in this module use dynamic allocation, but free
**	automatically all the memory reserved by them.
**
**	Therefore the caller never has to (and never should)
**	free() any object returned by these functions.
**
**	Therefore also all the strings returned by this package
**	are only valid until the next call to the same function
**	is made. This approach is selected, because of the nature
**	of access authorization: no string returned by the package
**	needs to be valid longer than until the next call.
**
**	This also makes it easy to plug the AA package in:
**	you don't have to ponder whether to free() something
**	here or is it done somewhere else (because it is always
**	done somewhere else).
**
**	The strings that the package needs to store are copied
**	so the original strings given as parameters to AA
**	functions may be freed or modified with no side effects.
**
**	The AA package does not free() anything else than what
**	it has itself allocated.
**
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
**
** HISTORY:
**	Oct 17	AL	Made corrections suggested by marca:
**			Added  if (!realm->username) return NULL;
**			Changed some ""s to NULLs.
**			Now doing calloc() to init uuencode source;
**			otherwise HTUU_encode() reads uninitialized memory
**			every now and then (not a real bug but not pretty).
**			Corrected the formula for uuencode destination size.
** BUGS:
**
**
*/

#include <string.h>		/* strchr() */

#include "HTUtils.h"
#include "HTString.h"
#include "HTParse.h"		/* URL parsing function		*/
#include "HTList.h"		/* HTList object		*/
#include "HTAlert.h"		/* HTConfirm(), HTPrompt()	*/
#include "HTAAUtil.h"		/* AA common to both sides	*/
#include "HTAssoc.h"		/* Assoc list			*/
#include "HTAABrow.h"		/* Implemented here		*/
#include "HTUU.h"		/* Uuencoding and uudecoding	*/
#include "HTAccess.h"		/* HTRequest structure		*/


/*
** Module-wide global variables
*/

PRIVATE HTList *server_table	= NULL;	/* Browser's info about servers	     */


/**************************** HTAAServer ***********************************/


/* PRIVATE						HTAAServer_new()
**		ALLOCATE A NEW NODE TO HOLD SERVER INFO
**		AND ADD IT TO THE LIST OF SERVERS
** ON ENTRY:
**	hostname	is the name of the host that the server
**			is running in.
**	portnumber	is the portnumber which the server listens.
**
** ON EXIT:
**	returns		the newly-allocated node with all the strings
**			duplicated.
**			Strings will be automatically freed by
**			the function HTAAServer_delete(), which also
**			frees the node itself.
*/
PRIVATE HTAAServer *HTAAServer_new ARGS2(CONST char*,	hostname,
					 int,		portnumber)
{
    HTAAServer *server;

    if (!(server = (HTAAServer *)malloc(sizeof(HTAAServer))))
	outofmem(__FILE__, "HTAAServer_new");

    server->hostname	= NULL;
    server->portnumber	= (portnumber > 0 ? portnumber : 80);
    server->setups	= HTList_new();
    server->realms	= HTList_new();

    if (hostname) StrAllocCopy(server->hostname, hostname);

    if (!server_table) server_table = HTList_new();

    HTList_addObject(server_table, (void*)server);

    return server;
}


/* PRIVATE						HTAAServer_delete()
**
**	DELETE THE ENTRY FOR THE SERVER FROM THE HOST TABLE,
**	AND FREE THE MEMORY USED BY IT.
**
** ON ENTRY:
**	killme		points to the HTAAServer to be freed.
**
** ON EXIT:
**	returns		nothing.
*/
#ifdef NOT_NEEDED_IT_SEEMS
PRIVATE void HTAASetup_delete();	/* Forward */
PRIVATE void HTAAServer_delete ARGS1(HTAAServer *, killme)
{
    if (killme) {
	HTList *cur1 = killme->setups;
	HTList *cur2 = killme->realms;
	HTAASetup *setup;
	HTAARealm *realm;

	while (NULL != (setup = (HTAASetup*)HTList_nextObject(cur1)))
	    HTAASetup_delete(setup);
	HTList_delete(killme->setups);

	while (NULL != (realm = (HTAARealm*)HTList_nextObject(cur2)))
	    ;	/* This sould free() the realm */
	HTList_delete(killme->realms);

	FREE(killme->hostname);

	HTList_removeObject(server_table, (void*)killme);

	free(killme);
    }
}
#endif /*NOT_NEEDED_IT_SEEMS*/


/* PRIVATE						HTAAServer_lookup()
**		LOOK UP SERVER BY HOSTNAME AND PORTNUMBER
** ON ENTRY:
**	hostname	obvious.
**	portnumber	if non-positive defaults to 80.
**
**	Looks up the server in the module-global server_table.
**
** ON EXIT:
**	returns		pointer to a HTAAServer structure
**			representing the looked-up server.
**			NULL, if not found.
*/
PRIVATE HTAAServer *HTAAServer_lookup ARGS2(CONST char *, hostname,
					    int,	  portnumber)
{
    if (hostname) {
	HTList *cur = server_table;
	HTAAServer *server;

	if (portnumber <= 0) portnumber = 80;

	while (NULL != (server = (HTAAServer*)HTList_nextObject(cur))) {
	    if (server->portnumber == portnumber  &&
		0==strcmp(server->hostname, hostname))
		return server;
	}
    }
    return NULL;	/* NULL parameter, or not found */
}




/*************************** HTAASetup *******************************/


/* PRIVATE						HTAASetup_lookup()
**	FIGURE OUT WHICH AUTHENTICATION SETUP THE SERVER
**	IS USING FOR A GIVEN FILE ON A GIVEN HOST AND PORT
**
** ON ENTRY:
**	hostname	is the name of the server host machine.
**	portnumber	is the port that the server is running in.
**	docname		is the (URL-)pathname of the document we
**			are trying to access.
**
** 	This function goes through the information known about
**	all the setups of the server, and finds out if the given
**	filename resides in one of the protected directories.
**
** ON EXIT:
**	returns		NULL if no match.
**			Otherwise, a HTAASetup structure representing
**			the protected server setup on the corresponding
**			document tree.
**
*/
PRIVATE HTAASetup *HTAASetup_lookup ARGS3(CONST char *, hostname,
					  int,		portnumber,
					  CONST char *, docname)
{
    HTAAServer *server;
    HTAASetup *setup;

    if (portnumber <= 0) portnumber = 80;

    if (hostname && docname && *hostname && /* *docname && */
	NULL != (server = HTAAServer_lookup(hostname, portnumber))) {

	HTList *cur = server->setups;

	if (TRACE) fprintf(stderr, "%s (%s:%d:%s)\n",
			   "HTAASetup_lookup: resolving setup for",
			   hostname, portnumber, docname);

	while (NULL != (setup = (HTAASetup*)HTList_nextObject(cur))) {
	    if (HTAA_templateMatch(setup->_template, docname)) {
		if (TRACE) fprintf(stderr, "%s `%s' %s `%s'\n",
				   "HTAASetup_lookup:", docname,
				   "matched template", setup->_template);
		return setup;
	    }
	    else if (TRACE) fprintf(stderr, "%s `%s' %s `%s'\n",
				    "HTAASetup_lookup:", docname,
				    "did NOT match template", setup->_template);
	} /* while setups remain */
    } /* if valid parameters and server found */

    if (TRACE) fprintf(stderr, "%s `%s' %s\n",
		       "HTAASetup_lookup: No template matched",
		       (docname ? docname : "(null)"),
		       "(so probably not protected)");

    return NULL;	/* NULL in parameters, or not found */
}




/* PRIVATE						HTAASetup_new()
**			CREATE A NEW SETUP NODE
** ON ENTRY:
**	server		is a pointer to a HTAAServer structure
**			to which this setup belongs.
**	template	documents matching this template
**			are protected according to this setup.
**	valid_schemes	a list containing all valid authentication
**			schemes for this setup.
**			If NULL, all schemes are disallowed.
**	scheme_specifics is an array of assoc lists, which
**			contain scheme specific parameters given
**			by server in Authenticate: fields.
**			If NULL, all scheme specifics are
**			set to NULL.
** ON EXIT:
**	returns		a new HTAASetup node, and also adds it as
**			part of the HTAAServer given as parameter.
*/
PRIVATE HTAASetup *HTAASetup_new ARGS4(HTAAServer *,	server,
				       char *,		_template,
				       HTList *,	valid_schemes,
				       HTAssocList **,	scheme_specifics)
{
    HTAASetup *setup;

    if (!server || !_template || !*_template) return NULL;

    if (!(setup = (HTAASetup*)malloc(sizeof(HTAASetup))))
	outofmem(__FILE__, "HTAASetup_new");

    setup->reprompt = NO;
    setup->server = server;
    setup->_template = NULL;
    if (_template) StrAllocCopy(setup->_template, _template);
    setup->valid_schemes = valid_schemes;
    setup->scheme_specifics = scheme_specifics;

    HTList_addObject(server->setups, (void*)setup);

    return setup;
}



/* PRIVATE						HTAASetup_delete()
**			FREE A HTAASetup STRUCTURE
** ON ENTRY:
**	killme		is a pointer to the structure to free().
**
** ON EXIT:
**	returns		nothing.
*/
#ifdef NOT_NEEDED_IT_SEEMS
PRIVATE void HTAASetup_delete ARGS1(HTAASetup *, killme)
{
    int scheme;

    if (killme) {
	if (killme->_template) free(killme->_template);
	if (killme->valid_schemes)
	    HTList_delete(killme->valid_schemes);
	for (scheme=0; scheme < HTAA_MAX_SCHEMES; scheme++)
	    if (killme->scheme_specifics[scheme])
		HTAssocList_delete(killme->scheme_specifics[scheme]);
	free(killme);
    }
}
#endif /*NOT_NEEDED_IT_SEEMS*/



/* PRIVATE					HTAASetup_updateSpecifics()
*		COPY SCHEME SPECIFIC PARAMETERS
**		TO HTAASetup STRUCTURE
** ON ENTRY:
**	setup		destination setup structure.
**	specifics	string array containing scheme
**			specific parameters for each scheme.
**			If NULL, all the scheme specific
**			parameters are set to NULL.
**
** ON EXIT:
**	returns		nothing.
*/
PRIVATE void HTAASetup_updateSpecifics ARGS2(HTAASetup *,	setup,
					     HTAssocList **,	specifics)
{
    int scheme;

    if (setup) {
	if (setup->scheme_specifics) {
	    for (scheme=0; scheme < HTAA_MAX_SCHEMES; scheme++) {
		if (setup->scheme_specifics[scheme])
		    HTAssocList_delete(setup->scheme_specifics[scheme]);
	    }
	    free(setup->scheme_specifics);
	}
	setup->scheme_specifics = specifics;
    }
}




/*************************** HTAARealm **********************************/

/* PRIVATE 						HTAARealm_lookup()
**		LOOKUP HTAARealm STRUCTURE BY REALM NAME
** ON ENTRY:
**	realm_table	a list of realm objects.
**	realmname	is the name of realm to look for.
**
** ON EXIT:
**	returns		the realm.  NULL, if not found.
*/
PRIVATE HTAARealm *HTAARealm_lookup ARGS2(HTList *,	realm_table,
					  CONST char *, realmname)
{
    if (realm_table && realmname) {
	HTList *cur = realm_table;
	HTAARealm *realm;

	while (NULL != (realm = (HTAARealm*)HTList_nextObject(cur))) {
	    if (0==strcmp(realm->realmname, realmname))
		return realm;
	}
    }
    return NULL;	/* No table, NULL param, or not found */
}



/* PRIVATE						HTAARealm_new()
**		CREATE A NODE CONTAINING USERNAME AND
**		PASSWORD USED FOR THE GIVEN REALM.
**		IF REALM ALREADY EXISTS, CHANGE
**		USERNAME/PASSWORD.
** ON ENTRY:
**	realm_table	a list of realms to where to add
**			the new one, too.
**	realmname	is the name of the password domain.
**	username	and
**	password	are what you can expect them to be.
**
** ON EXIT:
**	returns		the created realm.
*/
PRIVATE HTAARealm *HTAARealm_new(HTList *realm_table,
				       CONST char *realmname,
				       CONST char *username,
				       CONST char *password,
				       BOOL proxy)
{
    HTAARealm *realm;

    realm = HTAARealm_lookup(realm_table, realmname);

    if (!realm) {
	if (!(realm = (HTAARealm*)malloc(sizeof(HTAARealm))))
	    outofmem(__FILE__, "HTAARealm_new");
	realm->realmname = NULL;
	realm->username = NULL;
	realm->password = NULL;
	realm->success = NO;
	realm->proxy = proxy;
	StrAllocCopy(realm->realmname, realmname);
	if (realm_table) HTList_addObject(realm_table, (void*)realm);
    }
    if (username) StrAllocCopy(realm->username, username);
    if (password) StrAllocCopy(realm->password, password);

    return realm;
}




/***************** Basic and Pubkey Authentication ************************/

/* PRIVATE						compose_Basic_auth()
**
**		COMPOSE Basic SCHEME AUTHENTICATION STRING
**
** ON ENTRY:
**	req		request, where
**	req->scheme	== HTAA_BASIC
**	req->realm	contains username and password.
**
** ON EXIT:
**	returns		a newly composed authorization string,
**			NULL, if something fails.
** NOTE:
**	Like throughout the entire AA package, no string or structure
**	returned by AA package needs to (or should) be freed.
**
*/

#if 0

PRIVATE char *compose_Basic_auth ARGS1(HTRequest *, req)
{
    static char *result = NULL;	/* Uuencoded presentation, the result */
    char *cleartext = NULL;	/* Cleartext presentation */
    int len;

    FREE(result);	/* From previous call */

    if (!req || req->scheme != HTAA_BASIC || !req->setup ||
	!req->setup->server)
	return NULL;

    if (!req->realm) {
	char *realmname;

	if (!req->setup || !req->setup->scheme_specifics ||
	    !(realmname =
	      HTAssocList_lookup(req->setup->scheme_specifics[HTAA_BASIC],
				 "realm")))
	    return NULL;

	req->realm = HTAARealm_lookup(req->setup->server->realms, realmname);
	if (!req->realm) {
	    req->realm = HTAARealm_new(req->setup->server->realms,
				       realmname, NULL, NULL);
	    return NULL;
	}
    }

    len = strlen(req->realm->username ? req->realm->username : "") +
	  strlen(req->realm->password ? req->realm->password : "") + 3;

    if (!(cleartext  = (char*)calloc(len, 1)))
	outofmem(__FILE__, "compose_Basic_auth");

    if (req->realm->username) strcpy(cleartext, req->realm->username);
    else *cleartext = (char)0;

    strcat(cleartext, ":");

    if (req->realm->password) strcat(cleartext, req->realm->password);

    if (!(result = (char*)malloc(4 * ((len+2)/3) + 1)))
	outofmem(__FILE__, "compose_Basic_auth");
    HTUU_encode((unsigned char *)cleartext, strlen(cleartext), result);
    free(cleartext);

    return result;
}

#else

PRIVATE void compose_Basic_auth( HTRequest *req, char *auth[2] )
{
    static char *result = NULL;	/* Uuencoded presentation, the result */
    char *cleartext = NULL;	/* Cleartext presentation */
    int len;

    FREE(result);	/* From previous call */

    if (!req || req->scheme != HTAA_BASIC || !req->setup ||
	!req->setup->server)
	return;

    if ( req->proxy_realm ) {

    len = strlen(req->proxy_realm->username ? req->proxy_realm->username : "") +
	  strlen(req->proxy_realm->password ? req->proxy_realm->password : "") + 3;

    if (!(cleartext  = (char*)calloc(len, 1)))
	outofmem(__FILE__, "compose_Basic_auth");

    if (req->proxy_realm->username) strcpy(cleartext, req->proxy_realm->username);
    else *cleartext = (char)0;

    strcat(cleartext, ":");

    if (req->proxy_realm->password) strcat(cleartext, req->proxy_realm->password);

    if (!(auth[0] = (char*)malloc(4 * ((len+2)/3) + 1)))
	outofmem(__FILE__, "compose_Basic_auth");
    HTUU_encode((unsigned char *)cleartext, strlen(cleartext), auth[0]);
    free(cleartext);

    }

    if ( req->realm ) {

    len = strlen(req->realm->username ? req->realm->username : "") +
	  strlen(req->realm->password ? req->realm->password : "") + 3;

    if (!(cleartext  = (char*)calloc(len, 1)))
	outofmem(__FILE__, "compose_Basic_auth");

    if (req->realm->username) strcpy(cleartext, req->realm->username);
    else *cleartext = (char)0;

    strcat(cleartext, ":");

    if (req->realm->password) strcat(cleartext, req->realm->password);

    if (!(auth[1] = (char*)malloc(4 * ((len+2)/3) + 1)))
	outofmem(__FILE__, "compose_Basic_auth");
    HTUU_encode((unsigned char *)cleartext, strlen(cleartext), auth[1]);
    free(cleartext);

    }
}

#endif

/* BROWSER PRIVATE					HTAA_selectScheme()
**		SELECT THE AUTHENTICATION SCHEME TO USE
** ON ENTRY:
**	setup	is the server setup structure which can
**		be used to make the decision about the
**		used scheme.
**
**	When new authentication methods are added to library
**	this function makes the decision about which one to
**	use at a given time.  This can be done by inspecting
**	environment variables etc.
**
**	Currently only searches for the first valid scheme,
**	and if nothing found suggests Basic scheme;
**
** ON EXIT:
**	returns	the authentication scheme to use.
*/
PRIVATE HTAAScheme HTAA_selectScheme ARGS1(HTAASetup *, setup)
{
    HTAAScheme scheme;

    if (setup && setup->valid_schemes) {
	for (scheme=HTAA_BASIC; scheme < HTAA_MAX_SCHEMES; scheme++)
	    if (-1 < HTList_indexOf(setup->valid_schemes, (void*)scheme))
		return scheme;
    }
    return HTAA_NONE;
}




/* BROWSER PUBLIC					HTAA_composeAuth()
**
**	COMPOSE Authorization: HEADER LINE CONTENTS
**	IF WE ALREADY KNOW THAT THE HOST REQUIRES AUTHENTICATION
**
** ON ENTRY:
**	req		request, which contains
**	req->setup	protection setup info on browser.
**	req->scheme	selected authentication scheme.
**	req->realm	for Basic scheme the username and password.
**
** ON EXIT:
**	returns	NO, if no authorization seems to be needed, and
**		req->authorization is NULL.
**		YES, if it has composed Authorization field,
**		in which case the result is in req->authorization,
**		e.g.
**
**		   "Basic AkRDIhEF8sdEgs72F73bfaS=="
*/
PUBLIC BOOL HTAA_composeAuth ARGS1(HTRequest *, req)
{
    /*char *auth_string = NULL;*/
    char *auth[2];
    static char *docname=0;
    static char *hostname=0;
    int portnumber;
    char *colon;
    char *gate = NULL;	/* Obsolite? */
    char *arg = NULL;
    char *realmname = NULL;
    BOOL ret = NO;
    HTAARealm *realm = NULL;

    auth[0] = auth[1] = NULL;

    FREE(hostname);	/* From previous call */
    FREE(docname);	/*	- " -	      */

    if (!req  ||  !req->anchor)
	return NO;

#ifdef OLD_CODE
    if (req->authorization) {
	CTRACE(stderr,
"HTAA_composeAuth: forwarding auth.info from client\nAuthorization: %s\n",
	       req->authorization);
	return YES;
    }
#endif

    arg = HTAnchor_physical(req->anchor);
    docname = HTParse(arg, "", PARSE_PATH);
    hostname = HTParse((gate ? gate : arg), "", PARSE_HOST);
    if (hostname &&
	NULL != (colon = strchr(hostname, ':'))) {
	*(colon++) = '\0';	/* Chop off port number */
	portnumber = atoi(colon);
    }
    else portnumber = 80;

    if (TRACE) fprintf(stderr,
		       "Composing Authorization for %s:%d/%s\n",
		       hostname, portnumber, docname);

#ifdef OLD_CODE
    if (current_portnumber != portnumber ||
	!current_hostname || !current_docname ||
	!hostname         || !docname         ||
	0 != strcmp(current_hostname, hostname) ||
	0 != strcmp(current_docname, docname)) {

	retry = NO;

	current_portnumber = portnumber;

	if (hostname) StrAllocCopy(current_hostname, hostname);
	else FREE(current_hostname);

	if (docname) StrAllocCopy(current_docname, docname);
	else FREE(current_docname);
    }
    else retry = YES;
#endif /*OLD_CODE*/

    if (!req->setup)
	req->setup = HTAASetup_lookup(hostname, portnumber, docname);
    if (!req->setup)
	return NO;

    if (req->scheme == HTAA_NONE || req->scheme == HTAA_UNKNOWN)
	req->scheme = HTAA_selectScheme(req->setup);

    realmname = HTAssocList_lookup(req->setup->scheme_specifics[req->scheme], "realm");
    realm = HTAARealm_lookup(req->setup->server->realms, realmname);
    if ( realm->proxy == YES ) {
       req->proxy_realm = realm;
    } else {
       req->realm = realm;
    }

    switch (req->scheme) {
      case HTAA_BASIC:
	/*auth_string = compose_Basic_auth(req);*/
	compose_Basic_auth(req,auth);
	break;
      case HTAA_PUBKEY:
      case HTAA_KERBEROS_V4:
	/* OTHER AUTHENTICATION ROUTINES ARE CALLED HERE */
      default:
	{
	    char msg[100];
	    sprintf(msg, "%s %s `%s'",
		    "This client doesn't know how to compose authentication",
		    "information for scheme", HTAAScheme_name(req->scheme));
	    HTAlert(msg);
	    /*auth_string = NULL;*/
	    auth[0] = auth[1] = NULL;
	}
    } /* switch scheme */

    req->setup->reprompt = NO;
/*
    proxy_flag = req->proxy_authorization &&
		 !strcmp( req->proxy_authorization, "required" ) ? 1 : 0;
*/
    FREE(req->authorization);	 /* Free from previous call, Henrik 14/03-94 */
    FREE(req->proxy_authorization);
/*
    if ( !auth_string )
       return NO;
*/
    if ( auth[0] ) {

    if (!(req->proxy_authorization = (char*)malloc(sizeof(char) * (strlen(auth[0])+40))))
       outofmem(__FILE__, "HTAA_composeAuth");
    strcpy(req->proxy_authorization, HTAAScheme_name(req->scheme));
    strcat(req->proxy_authorization, " ");
    strcat(req->proxy_authorization, auth[0]);
    free( auth[0] );
    ret = YES;
    }

    if ( auth[1] ) {

    if (!(req->authorization = (char*)malloc(sizeof(char) * (strlen(auth[1])+40))))
       outofmem(__FILE__, "HTAA_composeAuth");
    strcpy(req->authorization, HTAAScheme_name(req->scheme));
    strcat(req->authorization, " ");
    strcat(req->authorization, auth[1]);
    free( auth[1] );
    ret = YES;
    }

    return ret;
}


/* BROWSER OVERLOADED					HTPasswordDialog()
**
**		PROMPT USERNAME AND PASSWORD, AND MAKE A
**		CALLBACK TO FUNCTION HTLoadHTTP().
**
**	This function must be redifined by GUI clients, which
**	call HTLoadHTTP(req) when user presses "Ok".
**
** ON ENTRY:
**	req		request.
**	req->dialog_msg	prompting message.
**	req->setup	information about protections of this request.
**	req->realm	structure describing one password realm.
**			This function should only be called when
**			server has replied with a 401 (Unauthorized)
**			status code, and req structure has been filled
**			up according to server reply, especially the
**			req->valid_shemes list must have been set up
**			according to WWW-Authenticate: headers.
**	req->retry_callback
**			function to call when username and password
**			have been entered.
** ON EXIT:
**
**	returns	nothing.
**		Calls (*req->retry_callback)(req).
**
*/
PUBLIC BOOL HTPasswordDialog ARGS1(HTRequest *,	req)
{
    HTAARealm *realm = req->realm ? req->realm : req->proxy_realm;
    if (!req || !req->setup || !realm || !req->dialog_msg ||
	!req->retry_callback) {
	HTAlert("HTPasswordDialog() called with an illegal parameter");
	return NO;
    }
    if (req->setup->reprompt &&
	NO == HTConfirm("Authorization failed.  Retry?")) {
	return NO;
    } /* HTConfirm(...) == NO */
    else { /* re-ask username+password (if misspelled) */
	char *username = realm->username;
	char *password = NULL;

	if ( NO == HTPromptUsernameAndPassword(req->dialog_msg, &username, &password) )
	   return NO;

	if (realm->username) free(realm->username);
	if (realm->password) free(realm->password);
	realm->username = username;
	realm->password = password;

	if (!realm->username)
	    return NO;		/* Suggested by marca; thanks! */

	(*req->retry_callback)(req);	/* Callback */
    }
    return YES;
}



/* BROWSER PUBLIC					HTAA_retryWithAuth()
**
**		RETRY THE SERVER WITH AUTHORIZATION (OR IF
**		ALREADY RETRIED, WITH A DIFFERENT USERNAME
**		AND/OR PASSWORD (IF MISSPELLED)) OR CANCEL
** ON ENTRY:
**	req		request.
**	req->valid_schemes
**			This function should only be called when
**			server has replied with a 401 (Unauthorized)
**			status code, and req structure has been filled
**			up according to server reply, especially the
**			req->valid_shemes list must have been set up
**			according to WWW-Authenticate: headers.
** ON EXIT:
**	On GUI clients pops up a username/password dialog box
**	with "Ok" and "Cancel".
**	"Ok" button press should do a callback
**
**		HTLoadHTTP(req);
**
**	This actually done by function HTPasswordDialog(),
**	which GUI clients redefine.
**
**	returns		YES, if dialog box was popped up.
**			NO, on failure.
*/
PUBLIC BOOL HTAA_retryWithAuth ARGS3(HTRequest *,	  req,
				     HTRetryCallbackType, callback,
				     BOOL, proxy)
{
    int len;
    char *realmname;
    char *arg = NULL;
    HTAARealm *realm = NULL;

    if (!req || !req->anchor ||
	!req->valid_schemes || HTList_count(req->valid_schemes) == 0) {
	req->setup = NULL;
	return NO;
    }

    arg = HTAnchor_physical(req->anchor);

    if (req->setup && req->setup->server) {
	/* So we have already tried with authorization.	*/
	/* Either we don't have access or username or	*/
	/* password was misspelled.			*/

	/* Update scheme-specific parameters	*/
	/* (in case they have expired by chance).	*/
	HTAASetup_updateSpecifics(req->setup, req->scheme_specifics);
	req->scheme = HTAA_selectScheme(req->setup);
	if ( !req->proxy_realm || req->proxy_realm->success == NO || req->realm )
	   req->setup->reprompt = YES;
    }
    else { /* current_setup == NULL, i.e. we have a	 */
	   /* first connection to a protected server or  */
	   /* the server serves a wider set of documents */
	   /* than we expected so far.                   */

	static char *hostname=0;
	static char *docname=0;
	int portnumber;
	char *colon;
	HTAAServer *server;

	FREE(hostname);	/* From previous call */
	FREE(docname);	/*	- " -	      */

	docname = HTParse(arg, "", PARSE_PATH);
	hostname = HTParse(arg, "", PARSE_HOST);
	if (hostname &&
	    NULL != (colon = strchr(hostname, ':'))) {
	    *(colon++) = '\0';	/* Chop off port number */
	    portnumber = atoi(colon);
	}
	else portnumber = 80;

	if (TRACE) fprintf(stderr,
			   "HTAA_retryWithAuth: first retry of %s:%d/%s\n",
			   hostname, portnumber, docname);

	if (!(server = HTAAServer_lookup(hostname, portnumber))) {
	    server = HTAAServer_new(hostname, portnumber);
	}
/*	else {
	    HTAlert("Access without authorization denied -- retrying");
	}*/

	if (!req->prot_template)
	    req->prot_template = HTAA_makeProtectionTemplate(docname);
	req->setup = HTAASetup_new(server,
				   req->prot_template,
				   req->valid_schemes,
				   req->scheme_specifics);
	req->setup->reprompt = NO;
	req->scheme = HTAA_selectScheme(req->setup);

    } /* else current_setup == NULL */

    realmname = HTAssocList_lookup(req->setup->scheme_specifics[req->scheme],
				   "realm");
    if (!realmname)
	return NO;

    req->retry_callback = callback;	/* Set callback function */

    if ( proxy == YES ) {
       realm = req->proxy_realm;
       req->proxy_realm = HTAARealm_lookup(req->setup->server->realms, realmname);
       if ( !realm ) {
	  if ( req->proxy_realm && req->proxy_realm->success == YES ) {
	     (*req->retry_callback)(req);	/* Callback */
	     return YES;
	  }
       }
       if ( !req->proxy_realm )
	  req->proxy_realm = HTAARealm_new(req->setup->server->realms, realmname, NULL, NULL, YES);
       realm = req->proxy_realm;
    } else {
       realm = req->realm;
       req->realm = HTAARealm_lookup(req->setup->server->realms, realmname);
       if ( !realm ) {
	  if ( req->realm && req->realm->success == YES ) {
	     (*req->retry_callback)(req);	/* Callback */
	     return YES;
	  }
       }
       if ( !req->realm )
	  req->realm = HTAARealm_new(req->setup->server->realms, realmname, NULL, NULL, NO);
       realm = req->realm;
    }

    len = strlen(realmname) + 100;
    if (req->setup->server->hostname)
	len += strlen(req->setup->server->hostname);

    FREE(req->dialog_msg);	 /* Free from previous call, Henrik 14/03-94 */
    if (!(req->dialog_msg = (char*)malloc(len)))
	outofmem(__FILE__, "HTAA_retryWithAuth");
    if (!realm->username)
	sprintf(req->dialog_msg, "%s %s at %s",
		"Enter username for",
		realm->realmname,
		req->setup->server->hostname
		? req->setup->server->hostname : "??");
    else sprintf(req->dialog_msg, "%s %s at %s",
		 "Enter username for",
		 realm->realmname,
		 req->setup->server->hostname
		 ? req->setup->server->hostname : "??");

    return HTPasswordDialog(req);
/*    return YES; */
}


/* PUPLIC						HTAA_cleanup()
**
**	Free the memory used by the entries concerning Access Authorization
**	in the request structure and put all pointers to NULL
**	Henrik 14/03-94.
**
** ON ENTRY:
**	req		the request structure
**
** ON EXIT:
**	returns		nothing.
*/
PUBLIC void HTAACleanup ARGS1(HTRequest *, req)
{
    if (req) {
	FREE(req->authorization);
	FREE(req->prot_template);
	FREE(req->dialog_msg);
#ifdef OLD_CODE
	/* Should not be freed as they have become a part of a static
	   memory */
	if (req->valid_schemes) {
	    HTList_delete(req->valid_schemes);
	    req->valid_schemes = NULL;
	}
	if (req->scheme_specifics) {
	    int cnt;
	    for (cnt=0; cnt<HTAA_MAX_SCHEMES; cnt++) {
		if (req->scheme_specifics[cnt])
		    HTAssocList_delete(req->scheme_specifics[cnt]);
	    }
	    FREE(req->scheme_specifics);
	}
#endif /* OLD_CODE */
    }
    return;
}



