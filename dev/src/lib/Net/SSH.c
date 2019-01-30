#include <Std.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <IO/File.h>
#include <Util/TypedFunction.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

typedef struct {
	unsigned long int ti_module;
	unsigned long int ti_offset;
} tls_index;

void * __attribute__((__regparm__ (1))) ___tls_get_addr(tls_index *ti) {
}

SYMBOL($block, "block");

INITIAL() {
	ssh_threads_init();
}

typedef struct ssh_session_t {
	const Std$Type$t *Type;
	ssh_session Handle;
} ssh_session_t;

TYPE(SSHSessionT);

GLOBAL_FUNCTION(New, 0) {
	ssh_session_t *Session = new(ssh_session_t);
	Session->Type = SSHSessionT;
	Session->Handle = ssh_new();
	Result->Val = (Std$Object$t *)Session;
	return SUCCESS;
}

typedef struct ssh_option_t {
	const Std$Type$t *Type;
	enum ssh_options_e Value;
} ssh_option_t;

TYPE(SSHOptionT);
TYPE(SSHStringOptionT, SSHOptionT);
TYPE(SSHIntOptionT, SSHOptionT);
TYPE(SSHBoolOptionT, SSHOptionT);
TYPE(SSHSocketOptionT, SSHOptionT);
TYPE(SSHDataOptionT, SSHOptionT);
TYPE(SSHCallbackOptionT, SSHOptionT);
TYPE(SSHLogVerbosityOptionT, SSHOptionT);

ssh_option_t SSHOptionHost[] = {{SSHStringOptionT, SSH_OPTIONS_HOST}};
ssh_option_t SSHOptionPort[] = {{SSHIntOptionT, SSH_OPTIONS_PORT}};
ssh_option_t SSHOptionPortStr[] = {{SSHStringOptionT, SSH_OPTIONS_PORT_STR}};
ssh_option_t SSHOptionFD[] = {{SSHSocketOptionT, SSH_OPTIONS_FD}};
ssh_option_t SSHOptionUser[] = {{SSHStringOptionT, SSH_OPTIONS_USER}};
ssh_option_t SSHOptionSSHDir[] = {{SSHStringOptionT, SSH_OPTIONS_SSH_DIR}};
ssh_option_t SSHOptionIdentity[] = {{SSHStringOptionT, SSH_OPTIONS_IDENTITY}};
ssh_option_t SSHOptionAddIdentity[] = {{SSHStringOptionT, SSH_OPTIONS_ADD_IDENTITY}};
ssh_option_t SSHOptionKnownHosts[] = {{SSHStringOptionT, SSH_OPTIONS_KNOWNHOSTS}};
ssh_option_t SSHOptionTimeout[] = {{SSHIntOptionT, SSH_OPTIONS_TIMEOUT}};
ssh_option_t SSHOptionTimeoutUSec[] = {{SSHIntOptionT, SSH_OPTIONS_TIMEOUT_USEC}};
ssh_option_t SSHOptionLogVerbosity[] = {{SSHLogVerbosityOptionT, SSH_OPTIONS_LOG_VERBOSITY}};
ssh_option_t SSHOptionLogVerbosityStr[] = {{SSHStringOptionT, SSH_OPTIONS_LOG_VERBOSITY_STR}};
ssh_option_t SSHOptionCiphersCS[] = {{SSHStringOptionT, SSH_OPTIONS_CIPHERS_C_S}};
ssh_option_t SSHOptionCiphersSC[] = {{SSHStringOptionT, SSH_OPTIONS_CIPHERS_S_C}};
ssh_option_t SSHOptionCompressionCS[] = {{SSHStringOptionT, SSH_OPTIONS_COMPRESSION_C_S}};
ssh_option_t SSHOptionCompressionSC[] = {{SSHStringOptionT, SSH_OPTIONS_COMPRESSION_S_C}};
ssh_option_t SSHOptionProxyCommand[] = {{SSHStringOptionT, SSH_OPTIONS_PROXYCOMMAND}};
ssh_option_t SSHOptionBindAddr[] = {{SSHStringOptionT, SSH_OPTIONS_BINDADDR}};
ssh_option_t SSHOptionStrictHostKeyCheck[] = {{SSHBoolOptionT, SSH_OPTIONS_STRICTHOSTKEYCHECK}};
ssh_option_t SSHOptionCompression[] = {{SSHStringOptionT, SSH_OPTIONS_COMPRESSION}};
ssh_option_t SSHOptionCompressionLevel[] = {{SSHIntOptionT, SSH_OPTIONS_COMPRESSION_LEVEL}};
ssh_option_t SSHOptionKeyExchange[] = {{SSHStringOptionT, SSH_OPTIONS_KEY_EXCHANGE}};
ssh_option_t SSHOptionHostKeys[] = {{SSHStringOptionT, SSH_OPTIONS_HOSTKEYS}};
ssh_option_t SSHOptionGSSAPIServerIdentity[] = {{SSHStringOptionT, SSH_OPTIONS_GSSAPI_SERVER_IDENTITY}};
ssh_option_t SSHOptionGSSAPIClientIdentity[] = {{SSHStringOptionT, SSH_OPTIONS_GSSAPI_CLIENT_IDENTITY}};
ssh_option_t SSHOptionGSSAPIDelegateCredentials[] = {{SSHIntOptionT, SSH_OPTIONS_GSSAPI_DELEGATE_CREDENTIALS}};
ssh_option_t SSHOptionPasswordAuth[] = {{SSHBoolOptionT, SSH_OPTIONS_PASSWORD_AUTH}};
ssh_option_t SSHOptionPubkeyAuth[] = {{SSHBoolOptionT, SSH_OPTIONS_PUBKEY_AUTH}};
ssh_option_t SSHOptionKbdIntAuth[] = {{SSHBoolOptionT, SSH_OPTIONS_KBDINT_AUTH}};
ssh_option_t SSHOptionGSSAPIAuth[] = {{SSHBoolOptionT, SSH_OPTIONS_GSSAPI_AUTH}};
ssh_option_t SSHOptionGlobalKnownHosts[] = {{SSHStringOptionT, SSH_OPTIONS_GLOBAL_KNOWNHOSTS}};
ssh_option_t SSHOptionNoDelay[] = {{SSHBoolOptionT, SSH_OPTIONS_NODELAY}};

METHOD("set", TYP, SSHSessionT, TYP, SSHStringOptionT, TYP, Std$String$T) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	ssh_option_t *Option = (ssh_option_t *)Args[1].Val;
	if (ssh_options_set(Session->Handle, Option->Value, Std$String$flatten(Args[2].Val))) {
		Result->Val = Std$String$copy(ssh_get_error(Session->Handle));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	}
}

METHOD("set", TYP, SSHSessionT, TYP, SSHIntOptionT, TYP, Std$Integer$SmallT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	ssh_option_t *Option = (ssh_option_t *)Args[1].Val;
	int Value = Std$Integer$get_small(Args[2].Val);
	if (ssh_options_set(Session->Handle, Option->Value, &Value)) {
		Result->Val = Std$String$copy(ssh_get_error(Session->Handle));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	}
}

METHOD("set", TYP, SSHSessionT, TYP, SSHBoolOptionT, TYP, Std$Symbol$T) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	ssh_option_t *Option = (ssh_option_t *)Args[1].Val;
	int Value = Args[2].Val == $true;
	if (ssh_options_set(Session->Handle, Option->Value, &Value)) {
		Result->Val = Std$String$copy(ssh_get_error(Session->Handle));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	}
}

typedef struct ssh_log_verbosity_t {
	const Std$Type$t *Type;
	int Value;
} ssh_log_verbosity_t;

TYPE(SSHLogVerbosityT);

ssh_log_verbosity_t SSHLogVerbosityNoLog[] = {{SSHLogVerbosityT, SSH_LOG_NOLOG}};
ssh_log_verbosity_t SSHLogVerbosityRare[] = {{SSHLogVerbosityT, SSH_LOG_RARE}};
ssh_log_verbosity_t SSHLogVerbosityProtocol[] = {{SSHLogVerbosityT, SSH_LOG_PROTOCOL}};
ssh_log_verbosity_t SSHLogVerbosityPacket[] = {{SSHLogVerbosityT, SSH_LOG_PACKET}};
ssh_log_verbosity_t SSHLogVerbosityFunctions[] = {{SSHLogVerbosityT, SSH_LOG_FUNCTIONS}};

METHOD("set", TYP, SSHSessionT, TYP, SSHLogVerbosityOptionT, TYP, SSHLogVerbosityT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	ssh_option_t *Option = (ssh_option_t *)Args[1].Val;
	ssh_log_verbosity_t *Verbosity = (ssh_log_verbosity_t *)Args[2].Val;
	if (ssh_options_set(Session->Handle, Option->Value, &Verbosity->Value)) {
		Result->Val = Std$String$copy(ssh_get_error(Session->Handle));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	}
}

METHOD("connect", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	if (ssh_connect(Session->Handle) != SSH_OK) {
		Result->Val = Std$String$copy(ssh_get_error(Session->Handle));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	}
}

METHOD("disconnect", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	ssh_disconnect(Session->Handle);
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("parse_config", TYP, SSHSessionT, TYP, Std$String$T) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	const char *FileName = Std$String$flatten(FileName);
	if (ssh_options_parse_config(Session->Handle, FileName) != SSH_OK) {
		Result->Val = Std$String$copy(ssh_get_error(Session->Handle));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	}
}

typedef struct ssh_key_t {
	const Std$Type$t *Type;
	ssh_key Handle;
} ssh_key_t;

TYPE(SSHKeyT);

METHOD("get_server_publickey", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	ssh_key_t *Key = new(ssh_key_t);
	Key->Type = SSHKeyT;
	if (ssh_get_server_publickey(Session->Handle, &Key->Handle)) {
		Result->Val = Std$String$copy(ssh_get_error(Session->Handle));
		return MESSAGE;
	} else {
		Result->Val = (Std$Object$t *)Key;
		return SUCCESS;
	}
}

typedef struct ssh_known_host_t {
	const Std$Type$t *Type;
	const char *Name;
	enum ssh_known_hosts_e Value;
} ssh_known_host_t;

TYPE(SSHKnownHostT);

METHOD("@", TYP, SSHKnownHostT, VAL, Std$String$T) {
	ssh_known_host_t *KnownHost = (ssh_known_host_t *)Args[0].Val;
	Result->Val = Std$String$new(KnownHost->Name);
	return SUCCESS;
}

ssh_known_host_t SSHKnownHostOK[] = {{SSHKnownHostT, "ok", SSH_KNOWN_HOSTS_OK}};
ssh_known_host_t SSHKnownHostChanged[] = {{SSHKnownHostT, "changed", SSH_KNOWN_HOSTS_CHANGED}};
ssh_known_host_t SSHKnownHostOther[] = {{SSHKnownHostT, "other", SSH_KNOWN_HOSTS_OTHER}};
ssh_known_host_t SSHKnownHostUnknown[] = {{SSHKnownHostT, "unknown", SSH_KNOWN_HOSTS_UNKNOWN}};
ssh_known_host_t SSHKnownHostNotFound[] = {{SSHKnownHostT, "not_found", SSH_KNOWN_HOSTS_NOT_FOUND}};
ssh_known_host_t SSHKnownHostError[] = {{SSHKnownHostT, "error", SSH_KNOWN_HOSTS_ERROR}};

METHOD("has_known_hosts_entry", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	switch (ssh_session_has_known_hosts_entry(Session->Handle)) {
	case SSH_KNOWN_HOSTS_UNKNOWN:
		Result->Val = (Std$Object$t *)SSHKnownHostUnknown;
		break;
	case SSH_KNOWN_HOSTS_OK:
		Result->Val = (Std$Object$t *)SSHKnownHostOK;
		break;
	case SSH_KNOWN_HOSTS_CHANGED:
		Result->Val = (Std$Object$t *)SSHKnownHostChanged;
		break;
	case SSH_KNOWN_HOSTS_OTHER:
		Result->Val = (Std$Object$t *)SSHKnownHostOther;
		break;
	case SSH_KNOWN_HOSTS_NOT_FOUND:
		Result->Val = (Std$Object$t *)SSHKnownHostNotFound;
		break;
	case SSH_KNOWN_HOSTS_ERROR:
		Result->Val = (Std$Object$t *)SSHKnownHostError;
		break;
	}
	return SUCCESS;
}

METHOD("is_known_server", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	switch (ssh_session_is_known_server(Session->Handle)) {
	case SSH_KNOWN_HOSTS_UNKNOWN:
		Result->Val = (Std$Object$t *)SSHKnownHostUnknown;
		break;
	case SSH_KNOWN_HOSTS_OK:
		Result->Val = (Std$Object$t *)SSHKnownHostOK;
		break;
	case SSH_KNOWN_HOSTS_CHANGED:
		Result->Val = (Std$Object$t *)SSHKnownHostChanged;
		break;
	case SSH_KNOWN_HOSTS_OTHER:
		Result->Val = (Std$Object$t *)SSHKnownHostOther;
		break;
	case SSH_KNOWN_HOSTS_NOT_FOUND:
		Result->Val = (Std$Object$t *)SSHKnownHostNotFound;
		break;
	case SSH_KNOWN_HOSTS_ERROR:
		Result->Val = (Std$Object$t *)SSHKnownHostError;
		break;
	}
	return SUCCESS;
}

typedef struct ssh_auth_t {
	const Std$Type$t *Type;
	const char *Name;
	enum ssh_auth_e Value;
} ssh_auth_t;

TYPE(SSHAuthT);

METHOD("@", TYP, SSHAuthT, VAL, Std$String$T) {
	ssh_auth_t *Auth = (ssh_auth_t *)Args[0].Val;
	Result->Val = Std$String$new(Auth->Name);
	return SUCCESS;
}

ssh_auth_t SSHAuthError[] = {{SSHAuthT, "error", SSH_AUTH_ERROR}};
ssh_auth_t SSHAuthDenied[] = {{SSHAuthT, "denied", SSH_AUTH_DENIED}};
ssh_auth_t SSHAuthPartial[] = {{SSHAuthT, "partial", SSH_AUTH_PARTIAL}};
ssh_auth_t SSHAuthSuccess[] = {{SSHAuthT, "success", SSH_AUTH_SUCCESS}};
ssh_auth_t SSHAuthAgain[] = {{SSHAuthT, "again", SSH_AUTH_AGAIN}};
ssh_auth_t SSHAuthInfo[] = {{SSHAuthT, "info", SSH_AUTH_INFO}};

METHOD("userauth_agent", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	int Response = ssh_userauth_agent(Session->Handle, NULL);
	switch (Response) {
	case SSH_AUTH_ERROR:
		Result->Val = (Std$Object$t *)SSHAuthError;
		break;
	case SSH_AUTH_DENIED:
		Result->Val = (Std$Object$t *)SSHAuthDenied;
		break;
	case SSH_AUTH_PARTIAL:
		Result->Val = (Std$Object$t *)SSHAuthPartial;
		break;
	case SSH_AUTH_SUCCESS:
		Result->Val = (Std$Object$t *)SSHAuthSuccess;
		break;
	case SSH_AUTH_AGAIN:
		Result->Val = (Std$Object$t *)SSHAuthAgain;
		break;
	case SSH_AUTH_INFO:
		Result->Val = (Std$Object$t *)SSHAuthInfo;
		break;
	}
	return SUCCESS;
}

METHOD("userauth_gssapi", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	int Response = ssh_userauth_gssapi(Session->Handle);
	switch (Response) {
	case SSH_AUTH_ERROR:
		Result->Val = (Std$Object$t *)SSHAuthError;
		break;
	case SSH_AUTH_DENIED:
		Result->Val = (Std$Object$t *)SSHAuthDenied;
		break;
	case SSH_AUTH_PARTIAL:
		Result->Val = (Std$Object$t *)SSHAuthPartial;
		break;
	case SSH_AUTH_SUCCESS:
		Result->Val = (Std$Object$t *)SSHAuthSuccess;
		break;
	case SSH_AUTH_AGAIN:
		Result->Val = (Std$Object$t *)SSHAuthAgain;
		break;
	case SSH_AUTH_INFO:
		Result->Val = (Std$Object$t *)SSHAuthInfo;
		break;
	}
	return SUCCESS;
}

METHOD("userauth_kbdint", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	int Response = ssh_userauth_kbdint(Session->Handle, NULL, NULL);
	switch (Response) {
	case SSH_AUTH_ERROR:
		Result->Val = (Std$Object$t *)SSHAuthError;
		break;
	case SSH_AUTH_DENIED:
		Result->Val = (Std$Object$t *)SSHAuthDenied;
		break;
	case SSH_AUTH_PARTIAL:
		Result->Val = (Std$Object$t *)SSHAuthPartial;
		break;
	case SSH_AUTH_SUCCESS:
		Result->Val = (Std$Object$t *)SSHAuthSuccess;
		break;
	case SSH_AUTH_AGAIN:
		Result->Val = (Std$Object$t *)SSHAuthAgain;
		break;
	case SSH_AUTH_INFO:
		Result->Val = (Std$Object$t *)SSHAuthInfo;
		break;
	}
	return SUCCESS;
}

METHOD("userauth_kbdint_getinstruction", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	Result->Val = Std$String$copy(ssh_userauth_kbdint_getinstruction(Session->Handle));
	return SUCCESS;
}

METHOD("userauth_kbdint_getname", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	Result->Val = Std$String$copy(ssh_userauth_kbdint_getname(Session->Handle));
	return SUCCESS;
}

METHOD("userauth_kbdint_getnprompts", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(ssh_userauth_kbdint_getnprompts(Session->Handle));
	return SUCCESS;
}

METHOD("userauth_kbdint_getprompt", TYP, SSHSessionT, TYP, Std$Integer$SmallT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	char Echo;
	Result->Val = Std$String$copy(ssh_userauth_kbdint_getprompt(
		Session->Handle, Std$Integer$get_small(Args[1].Val), &Echo
	));
	if (Count > 2 && Args[2].Ref) Args[2].Ref[0] = Echo ? $true : $false;
	return SUCCESS;
}

METHOD("userauth_kbdint_setanswer", TYP, SSHSessionT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	if (ssh_userauth_kbdint_setanswer(
		Session->Handle, Std$Integer$get_small(Args[1].Val), Std$String$flatten(Args[2].Val)
	)) {
		Result->Val = Std$String$copy(ssh_get_error(Session->Handle));
		return MESSAGE;
	} else {
		Result->Arg = Args[0];
		return SUCCESS;
	}
}

METHOD("userauth_password", TYP, SSHSessionT, TYP, Std$String$T) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	int Response = ssh_userauth_password(Session->Handle, NULL, Std$String$flatten(Args[1].Val));
	switch (Response) {
	case SSH_AUTH_ERROR:
		Result->Val = (Std$Object$t *)SSHAuthError;
		break;
	case SSH_AUTH_DENIED:
		Result->Val = (Std$Object$t *)SSHAuthDenied;
		break;
	case SSH_AUTH_PARTIAL:
		Result->Val = (Std$Object$t *)SSHAuthPartial;
		break;
	case SSH_AUTH_SUCCESS:
		Result->Val = (Std$Object$t *)SSHAuthSuccess;
		break;
	case SSH_AUTH_AGAIN:
		Result->Val = (Std$Object$t *)SSHAuthAgain;
		break;
	case SSH_AUTH_INFO:
		Result->Val = (Std$Object$t *)SSHAuthInfo;
		break;
	}
	return SUCCESS;
}

METHOD("userauth_publickey", TYP, SSHSessionT, TYP, Std$String$T) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	int Response = ssh_userauth_publickey(Session->Handle, NULL, Std$String$flatten(Args[1].Val));
	switch (Response) {
	case SSH_AUTH_ERROR:
		Result->Val = (Std$Object$t *)SSHAuthError;
		break;
	case SSH_AUTH_DENIED:
		Result->Val = (Std$Object$t *)SSHAuthDenied;
		break;
	case SSH_AUTH_PARTIAL:
		Result->Val = (Std$Object$t *)SSHAuthPartial;
		break;
	case SSH_AUTH_SUCCESS:
		Result->Val = (Std$Object$t *)SSHAuthSuccess;
		break;
	case SSH_AUTH_AGAIN:
		Result->Val = (Std$Object$t *)SSHAuthAgain;
		break;
	case SSH_AUTH_INFO:
		Result->Val = (Std$Object$t *)SSHAuthInfo;
		break;
	}
	return SUCCESS;
}

typedef struct ssh_channel_t {
	const Std$Type$t *Type;
	ssh_channel Handle;
} ssh_channel_t;

TYPE(SSHChannelT);

METHOD("channel_new", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	ssh_channel_t *Channel = new(ssh_channel_t);
	Channel->Type = SSHChannelT;
	if (!(Channel->Handle = ssh_channel_new(Session->Handle))) {
		Result->Val = Std$String$new("Error creating channel");
		return MESSAGE;
	}
	Result->Val = (Std$Object$t *)Channel;
	return SUCCESS;
}

METHOD("close", TYP, SSHChannelT) {
	ssh_channel_t *Channel = (ssh_channel_t *)Args[0].Val;
	ssh_channel_close(Channel->Handle);
	int Response = ssh_channel_close(Channel->Handle);
	if (Response != SSH_OK) {
		Result->Val = Std$String$new("Error closing channel");
		return MESSAGE;
	}
	return SUCCESS;
}

METHOD("open_session", TYP, SSHChannelT) {
	ssh_channel_t *Channel = (ssh_channel_t *)Args[0].Val;
	int Response = ssh_channel_open_session(Channel->Handle);
	if (Response != SSH_OK) {
		Result->Val = Std$String$new("Error opening session");
		return MESSAGE;
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("poll", TYP, SSHChannelT) {
	ssh_channel_t *Channel = (ssh_channel_t *)Args[0].Val;
	int Response = ssh_channel_poll(Channel->Handle, (Count > 1) && (Args[1].Val == $true));
	if (Response == SSH_ERROR) {
		Result->Val = Std$String$new("Error polling channel");
		return MESSAGE;
	} else if (Response == 0) {
		return FAILURE;
	} else {
		Result->Val = Std$Integer$new_small(Response);
		return SUCCESS;
	}
}

METHOD("poll_timeout", TYP, SSHChannelT, TYP, Std$Integer$SmallT) {
	ssh_channel_t *Channel = (ssh_channel_t *)Args[0].Val;
	int Response = ssh_channel_poll(Channel->Handle, Std$Integer$get_small(Args[1].Val));
	if (Response == SSH_ERROR) {
		Result->Val = Std$String$new("Error polling channel");
		return MESSAGE;
	} else if (Response == 0) {
		return FAILURE;
	} else {
		Result->Val = Std$Integer$new_small(Response);
		return SUCCESS;
	}
}

METHOD("read", TYP, SSHChannelT, TYP, Std$Address$T, TYP, Std$Integer$T) {
	ssh_channel_t *Channel = (ssh_channel_t *)Args[0].Val;
	int Response = ssh_channel_read(Channel->Handle, Std$Address$get_value(Args[1].Val), Std$Integer$get_small(Args[2].Val), (Count > 3) && (Args[3].Val == $true));
	if (Response == SSH_ERROR) {
		Result->Val = Std$String$new("Error polling channel");
		return MESSAGE;
	} else if (Response == 0) {
		return FAILURE;
	} else {
		Result->Val = Std$Integer$new_small(Response);
		return SUCCESS;
	}
}

METHOD("read_nonblocking", TYP, SSHChannelT, TYP, Std$Address$T, TYP, Std$Integer$T) {
	ssh_channel_t *Channel = (ssh_channel_t *)Args[0].Val;
	int Response = ssh_channel_read_nonblocking(Channel->Handle, Std$Address$get_value(Args[1].Val), Std$Integer$get_small(Args[2].Val), (Count > 3) && (Args[3].Val == $true));
	if (Response == SSH_ERROR) {
		Result->Val = Std$String$new("Error polling channel");
		return MESSAGE;
	} else if (Response == 0) {
		return FAILURE;
	} else {
		Result->Val = Std$Integer$new_small(Response);
		return SUCCESS;
	}
}

METHOD("write", TYP, SSHChannelT, TYP, Std$Address$T, TYP, Std$Integer$T) {
	ssh_channel_t *Channel = (ssh_channel_t *)Args[0].Val;
	int Response = ssh_channel_write(Channel->Handle, Std$Address$get_value(Args[1].Val), Std$Integer$get_small(Args[2].Val));
	if (Response == SSH_ERROR) {
		Result->Val = Std$String$new("Error polling channel");
		return MESSAGE;
	} else if (Response == 0) {
		return FAILURE;
	} else {
		Result->Val = Std$Integer$new_small(Response);
		return SUCCESS;
	}
}

METHOD("write_stderr", TYP, SSHChannelT, TYP, Std$Address$T, TYP, Std$Integer$T) {
	ssh_channel_t *Channel = (ssh_channel_t *)Args[0].Val;
	int Response = ssh_channel_write_stderr(Channel->Handle, Std$Address$get_value(Args[1].Val), Std$Integer$get_small(Args[2].Val));
	if (Response == SSH_ERROR) {
		Result->Val = Std$String$new("Error polling channel");
		return MESSAGE;
	} else if (Response == 0) {
		return FAILURE;
	} else {
		Result->Val = Std$Integer$new_small(Response);
		return SUCCESS;
	}
}

typedef struct sftp_session_t {
	const Std$Type$t *Type;
	sftp_session Handle;
} sftp_session_t;

TYPE(SFTPSessionT);

METHOD("sftp_new", TYP, SSHSessionT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	sftp_session_t *SFTPSession = new(sftp_session_t);
	SFTPSession->Type = SFTPSessionT;
	if (!(SFTPSession->Handle = sftp_new(Session->Handle))) {
		Result->Val = Std$String$copy(ssh_get_error(Session->Handle));
		return MESSAGE;
	}
	Result->Val = (Std$Object$t *)SFTPSession;
	return SUCCESS;
}

METHOD("sftp_new_channel", TYP, SSHSessionT, TYP, SSHChannelT) {
	ssh_session_t *Session = (ssh_session_t *)Args[0].Val;
	ssh_channel_t *Channel = (ssh_channel_t *)Args[1].Val;
	sftp_session_t *SFTPSession = new(sftp_session_t);
	SFTPSession->Type = SFTPSessionT;
	if (!(SFTPSession->Handle = sftp_new_channel(Session->Handle, Channel->Handle))) {
		Result->Val = Std$String$copy(ssh_get_error(Session->Handle));
		return MESSAGE;
	}
	Result->Val = (Std$Object$t *)SFTPSession;
	return SUCCESS;
}

METHOD("init", TYP, SFTPSessionT) {
	sftp_session_t *Session = (sftp_session_t *)Args[0].Val;
	if (sftp_init(Session->Handle)) {
		Result->Val = Std$String$copy(sftp_get_error(Session->Handle));
		return MESSAGE;
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("free", TYP, SFTPSessionT) {
	sftp_session_t *Session = (sftp_session_t *)Args[0].Val;
	sftp_free(Session->Handle);
	Session->Handle = 0;
	return SUCCESS;
}

typedef struct sftp_file_t {
	const Std$Type$t *Type;
	sftp_file Handle;
} sftp_file_t;

typedef struct {const Std$Type$t *Type; int Flags;} openmode_t;

TYPE(SFTPFileT, IO$Stream$SeekerT, IO$Stream$T);

TYPE(SFTPFileReaderT, SFTPFileT, IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(SFTPFileWriterT, SFTPFileT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(SFTPFileReaderWriterT, SFTPFileReaderT, SFTPFileWriterT, SFTPFileT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);

TYPE(SFTPFileTextReaderT, SFTPFileReaderT, SFTPFileT, IO$Stream$TextReaderT, IO$Stream$ReaderT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(SFTPFileTextWriterT, SFTPFileWriterT, SFTPFileT, IO$Stream$TextWriterT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);
TYPE(SFTPFileTextReaderWriterT, SFTPFileReaderT, SFTPFileWriterT, SFTPFileT, IO$Stream$TextReaderT, IO$Stream$TextWriterT, IO$Stream$ReaderT, IO$Stream$WriterT, IO$Stream$SeekerT, IO$Stream$T);

static openmode_t OpenModes[] = {
	{SFTPFileT, 0}, {SFTPFileReaderT, O_RDONLY}, {SFTPFileWriterT, O_WRONLY | O_CREAT}, {SFTPFileReaderWriterT, O_RDWR | O_CREAT},
	{SFTPFileT, 0}, {SFTPFileTextReaderT, O_RDONLY}, {SFTPFileTextWriterT, O_WRONLY | O_CREAT}, {SFTPFileTextReaderWriterT, O_RDWR | O_CREAT}
};

static void sftp_file_finalize(sftp_file_t *Stream, void *Data) {
	sftp_close(Stream->Handle);
}

METHOD("open", TYP, SFTPSessionT, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	sftp_session_t *Session = (sftp_session_t *)Args[0].Val;
	const char *FileName = Std$String$flatten(Args[1].Val);
	int Flags = Std$Integer$get_small(Args[2].Val);
	openmode_t *OpenMode = &OpenModes[Flags % 8];
	int AccessType = OpenMode->Flags;
	if (Flags & IO$File$OPEN_EXCLUSIVE) AccessType |= O_EXCL;
	if (Flags & IO$File$OPEN_TRUNCATE) AccessType |= O_TRUNC;
	sftp_file_t *Stream = new(sftp_file_t);
	Stream->Type = OpenMode->Type;
	if (!(Stream->Handle = sftp_open(Session->Handle, FileName, AccessType, 0644))) {
		Result->Val = Std$String$copy(sftp_get_error(Session->Handle));
		return MESSAGE;
	}
	if (Flags & IO$File$OPEN_NOBLOCK) {
		sftp_file_set_nonblocking(Stream->Handle);
	}
	if (Flags & IO$File$OPEN_APPEND) {
		sftp_attributes Attributes = sftp_stat(Session->Handle, FileName);
		if (Attributes) {
			sftp_seek64(Stream->Handle, Attributes->size);
			sftp_attributes_free(Attributes);
		}
	}
	Result->Val = (Std$Object$t *)Stream;
	Riva$Memory$register_finalizer((void *)Stream, (void *)sftp_file_finalize, 0, 0, 0);
	return SUCCESS;
}

TYPED_INSTANCE(int, IO$Stream$read, SFTPFileReaderT, sftp_file_t *Stream, char * restrict Buffer, int Count, int Block) {
	if (Block) {
		int Total = 0;
		while (Count) {
			int Bytes = sftp_read(Stream->Handle, Buffer, Count);
			if (Bytes == 0) return Total;
			if (Bytes == -1) return -1;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		}
		return Total;
	} else {
		return sftp_read(Stream->Handle, Buffer, Count);
	}
}

TYPED_INSTANCE(int, IO$Stream$write, SFTPFileWriterT, sftp_file_t *Stream, const char *Buffer, int Count, int Blocks) {
	if (Blocks) {
		int Total = 0;
		while (Count) {
			int Bytes = sftp_write(Stream->Handle, Buffer, Count);
			if (Bytes == -1) return -1;
			if (Bytes == 0) return Total;
			Total += Bytes;
			Buffer += Bytes;
			Count -= Bytes;
		}
		return Total;
	} else {
		return sftp_write(Stream->Handle, Buffer, Count);
	}
}

METHOD("close", TYP, SFTPFileT) {
	sftp_file_t *Stream = (sftp_file_t *)Args[0].Val;
	sftp_close(Stream->Handle);
	Riva$Memory$register_finalizer((void *)Stream, 0, 0, 0, 0);
	return SUCCESS;
}

METHOD("read", TYP, SFTPFileReaderT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	sftp_file_t *Stream = (sftp_file_t *)Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Integer$get_small(Args[2].Val);
	int BytesRead = sftp_read(Stream->Handle, Buffer, Size);
	if (BytesRead < 0) {
		Result->Val = (Std$Object_t *)IO$Stream$ReadMessage;
		return MESSAGE;
	}
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
}

METHOD("read", TYP, SFTPFileReaderT, TYP, Std$Address$T, TYP, Std$Integer$SmallT, VAL, $block) {
	sftp_file_t *Stream = (sftp_file_t *)Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Integer$get_small(Args[2].Val);
	int BytesRead = 0;
	while (Size) {
		int Request = Size <= 16384 ? Size : 16384;
		int Bytes = sftp_read(Stream->Handle, Buffer, Request);
		if (BytesRead < 0) {
			Result->Val = (Std$Object_t *)IO$Stream$ReadMessage;
			return MESSAGE;
		}
		if (Bytes == 0) break;
		BytesRead += Bytes;
		Buffer += Bytes;
		Size -= Bytes;
	}
	Result->Val = Std$Integer$new_small(BytesRead);
	return SUCCESS;
}

METHOD("write", TYP, SFTPFileWriterT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	sftp_file_t *Stream = (sftp_file_t *)Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Integer$get_small(Args[2].Val);
	int BytesWritten = sftp_write(Stream->Handle, Buffer, Size);
	if (BytesWritten < 0) {
		Result->Val = (Std$Object_t *)IO$Stream$WriteMessage;
		return MESSAGE;
	}
	Result->Val = Std$Integer$new_small(BytesWritten);
	return SUCCESS;
}

METHOD("write", TYP, SFTPFileWriterT, TYP, Std$Address$T, TYP, Std$Integer$SmallT, VAL, $block) {
	sftp_file_t *File = (sftp_file_t *)Args[0].Val;
	char *Buffer = Std$Address$get_value(Args[1].Val);
	int Size = Std$Integer$get_small(Args[2].Val);
	int BytesWritten = 0;
	while (Size) {
		int Request = Size <= 16384 ? Size : 16384;
		int Bytes = sftp_write(File->Handle, Buffer, Request);
		if (Bytes < 0) {
			Result->Val = (Std$Object_t *)IO$Stream$WriteMessage;
			return MESSAGE;
		}
		if (Bytes == 0) break;
		BytesWritten += Bytes;
		Buffer += Bytes;
		Size -= Bytes;
	}
	Result->Val = Std$Integer$new_small(BytesWritten);
	return SUCCESS;
}

typedef struct listdir_generator {
	Std$Function$cstate State;
	sftp_session *Session;
	sftp_dir *Dir;
} listdir_generator;

static void listdir_finalize(listdir_generator *Generator, void *Arg) {
	if (Generator) sftp_closedir(Generator->Dir);
}

static long listdir_resume(Std$Function$result *Result) {
	listdir_generator *Generator = (listdir_generator *)Result->State;
	sftp_attributes Attributes = sftp_readdir(Generator->Session, Generator->Dir);
	if (!Attributes) {
		sftp_closedir(Generator->Dir);
		Generator->Dir = 0;
		Riva$Memory$register_finalizer(Generator, 0, 0, 0, 0);
		return FAILURE;
	}
	Result->Val = Std$String$copy(Attributes->name);
	sftp_attributes_free(Attributes);
	return SUSPEND;
}

METHOD("lsdir", TYP, SFTPSessionT, TYP, Std$String$T) {
	sftp_session_t *Session = (sftp_session_t *)Args[0].Val;
	const char *Path = Std$String$flatten(Args[1].Val);
	sftp_dir *Dir = sftp_opendir(Session->Handle, Path);
	if (!Dir) {
		Result->Val = Std$String$copy(sftp_get_error(Session->Handle));
		return MESSAGE;
	}
	sftp_attributes Attributes = sftp_readdir(Session->Handle, Dir);
	if (!Attributes) {
		closedir(Dir);
		return FAILURE;
	}
	Result->Val = Std$String$copy(Attributes->name);
	sftp_attributes_free(Attributes);
	listdir_generator *Generator = new(listdir_generator);
	Riva$Memory$register_finalizer(Generator, listdir_finalize, 0, 0, 0);
	if (Generator->State.Chain) Riva$Memory$register_disappearing_link(&Generator->State.Chain, Generator->State.Chain);
	Generator->State.Run = Std$Function$resume_c;
	Generator->State.Invoke = listdir_resume;
	Generator->Session = Session->Handle;
	Generator->Dir = Dir;
	Result->State = Generator;
	return SUSPEND;
}

typedef struct sftp_attributes_t {
	const Std$Type$t *Type;
	sftp_attributes Handle;
} sftp_attributes_t;

TYPE(SFTPAttributesT);

METHOD("name", TYP, SFTPAttributesT) {
	sftp_attributes_t *Attributes = (sftp_attributes_t *)Args[0].Val;
	Result->Val = Std$String$copy(Attributes->Handle->name);
	return SUCCESS;
}

METHOD("flags", TYP, SFTPAttributesT) {
	sftp_attributes_t *Attributes = (sftp_attributes_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Attributes->Handle->flags);
	return SUCCESS;
}

typedef struct sftp_file_type_t {
	const Std$Type$t *Type;
	const char *Name;
	int Value;
} sftp_file_type_t;

TYPE(SFTPFileTypeT);

sftp_file_type_t SFTPFileTypeRegular[] = {{SFTPFileTypeT, "regular", SSH_FILEXFER_TYPE_REGULAR}};
sftp_file_type_t SFTPFileTypeDirectory[] = {{SFTPFileTypeT, "directory", SSH_FILEXFER_TYPE_DIRECTORY}};
sftp_file_type_t SFTPFileTypeSymLink[] = {{SFTPFileTypeT, "symlink", SSH_FILEXFER_TYPE_SYMLINK}};
sftp_file_type_t SFTPFileTypeSpecial[] = {{SFTPFileTypeT, "special", SSH_FILEXFER_TYPE_SPECIAL}};
sftp_file_type_t SFTPFileTypeUnknown[] = {{SFTPFileTypeT, "unknown", SSH_FILEXFER_TYPE_UNKNOWN}};

METHOD("@", TYP, SFTPFileTypeT, VAL, Std$String$T) {
	sftp_file_type_t *FileType = (sftp_file_type_t *)Args[0].Val;
	Result->Val = Std$String$new(FileType->Name);
	return SUCCESS;
}

METHOD("type", TYP, SFTPAttributesT) {
	sftp_attributes_t *Attributes = (sftp_attributes_t *)Args[0].Val;
	switch (Attributes->Handle->type) {
	case SSH_FILEXFER_TYPE_REGULAR:
		Result->Val = (Std$Object$t *)SFTPFileTypeRegular;
		break;
	case SSH_FILEXFER_TYPE_DIRECTORY:
		Result->Val = (Std$Object$t *)SFTPFileTypeDirectory;
		break;
	case SSH_FILEXFER_TYPE_SYMLINK:
		Result->Val = (Std$Object$t *)SFTPFileTypeSymLink;
		break;
	case SSH_FILEXFER_TYPE_SPECIAL:
		Result->Val = (Std$Object$t *)SFTPFileTypeSpecial;
		break;
	case SSH_FILEXFER_TYPE_UNKNOWN:
		Result->Val = (Std$Object$t *)SFTPFileTypeUnknown;
		break;
	}
	return SUCCESS;
}
METHOD("size", TYP, SFTPAttributesT) {
	sftp_attributes_t *Attributes = (sftp_attributes_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Attributes->Handle->size);
	return SUCCESS;
}
METHOD("uid", TYP, SFTPAttributesT) {
	sftp_attributes_t *Attributes = (sftp_attributes_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Attributes->Handle->uid);
	return SUCCESS;
}
METHOD("gid", TYP, SFTPAttributesT) {
	sftp_attributes_t *Attributes = (sftp_attributes_t *)Args[0].Val;
	Result->Val = Std$Integer$new_small(Attributes->Handle->gid);
	return SUCCESS;
}
METHOD("owner", TYP, SFTPAttributesT) {
	sftp_attributes_t *Attributes = (sftp_attributes_t *)Args[0].Val;
	Result->Val = Std$String$copy(Attributes->Handle->owner);
	return SUCCESS;
}
METHOD("group", TYP, SFTPAttributesT) {
	sftp_attributes_t *Attributes = (sftp_attributes_t *)Args[0].Val;
	Result->Val = Std$String$copy(Attributes->Handle->group);
	return SUCCESS;
}

static long listdirinfo_resume(Std$Function$result *Result) {
	listdir_generator *Generator = (listdir_generator *)Result->State;
	sftp_attributes_t *Attributes = new(sftp_attributes_t);
	Attributes->Type = SFTPAttributesT;
	if (!(Attributes->Handle = sftp_readdir(Generator->Session, Generator->Dir))) {
		sftp_closedir(Generator->Dir);
		Generator->Dir = 0;
		Riva$Memory$register_finalizer(Generator, 0, 0, 0, 0);
		return FAILURE;
	}
	Result->Val = (Std$Object$t *)Attributes;
	return SUSPEND;
}

METHOD("lsdirinfo", TYP, SFTPSessionT, TYP, Std$String$T) {
	sftp_session_t *Session = (sftp_session_t *)Args[0].Val;
	const char *Path = Std$String$flatten(Args[1].Val);
	sftp_dir *Dir = sftp_opendir(Session->Handle, Path);
	if (!Dir) {
		Result->Val = Std$String$copy(sftp_get_error(Session->Handle));
		return MESSAGE;
	}
	sftp_attributes_t *Attributes = new(sftp_attributes_t);
	Attributes->Type = SFTPAttributesT;
	if (!(Attributes->Handle = sftp_readdir(Session->Handle, Dir))) {
		closedir(Dir);
		return FAILURE;
	}
	Result->Val = (Std$Object$t *)Attributes;
	listdir_generator *Generator = new(listdir_generator);
	Riva$Memory$register_finalizer(Generator, listdir_finalize, 0, 0, 0);
	if (Generator->State.Chain) Riva$Memory$register_disappearing_link(&Generator->State.Chain, Generator->State.Chain);
	Generator->State.Run = Std$Function$resume_c;
	Generator->State.Invoke = listdirinfo_resume;
	Generator->Session = Session->Handle;
	Generator->Dir = Dir;
	Result->State = Generator;
	return SUSPEND;
}
