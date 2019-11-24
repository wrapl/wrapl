#include <Std.h>
#include <Riva.h>
#include <Agg/List.h>
#include <Sys/Time.h>
#include <Sys/Module.h>
#include <IO/Stream.h>

#include <curl/curl.h>

typedef struct share_t {
	const Std$Type$t *Type;
	CURLSH *Handle;
} share_t;

typedef struct curl_t {
	const Std$Type$t *Type;
	CURL *Handle;
} curl_t;

typedef struct multi_t {
	const Std$Type$t *Type;
	CURLM *Handle;
} multi_t;

typedef struct curl_option_t {
	const Std$Type$t *Type;
	CURLoption Value;
} curl_option_t;

typedef struct curl_info_t {
	const Std$Type$t *Type;
	CURLINFO Value;
} curl_info_t;

TYPE(ShareT);
TYPE(T);
TYPE(MultiT);
TYPE(OptionT);
TYPE(InfoT);

static inline void register_option(Sys$Module$t *Module, char *Name, CURLoption Value) {
	curl_option_t *Option = new(curl_option_t);
	Option->Type = OptionT;
	Option->Value = Value;
	Sys$Module$export(Module, Name, 0, Option);
}

#define FAKEOPT_WRITE_STREAM CURLOPT_LASTENTRY + 1
#define FAKEOPT_READ_STREAM CURLOPT_LASTENTRY + 2

static curl_option_t WriteStreamOption[] = {{OptionT, FAKEOPT_WRITE_STREAM}};
static curl_option_t ReadStreamOption[] = {{OptionT, FAKEOPT_READ_STREAM}};

static inline void register_fake_option(Sys$Module$t *Module, char *Name, curl_option_t *Option) {
	Sys$Module$export(Module, Name, 0, Option);
}

CONSTANT(Option, Sys$Module$T) {
	Sys$Module$t *Module = Sys$Module$new("Option");
	Sys$Module$export(Module, "T", 0, OptionT);
	register_option(Module, "Verbose", CURLOPT_VERBOSE);
	register_option(Module, "Header", CURLOPT_HEADER);
	register_option(Module, "NoProgress", CURLOPT_NOPROGRESS);
	register_option(Module, "NoSignal", CURLOPT_NOSIGNAL);
	register_option(Module, "WriteFunc", CURLOPT_WRITEFUNCTION);
	register_option(Module, "WriteData", CURLOPT_WRITEDATA);
	register_option(Module, "ReadFunc", CURLOPT_READFUNCTION);
	register_option(Module, "ReadData", CURLOPT_READDATA);
	register_option(Module, "IoctlFunc", CURLOPT_IOCTLFUNCTION);
	register_option(Module, "IoctlData", CURLOPT_IOCTLDATA);
	register_option(Module, "XferInfoFunc", CURLOPT_XFERINFOFUNCTION);
	register_option(Module, "XferInfoData", CURLOPT_XFERINFODATA);
#ifdef LINUX
	register_option(Module, "SeekFunc", CURLOPT_SEEKFUNCTION);
	register_option(Module, "SeekData", CURLOPT_SEEKDATA);
#endif
	register_option(Module, "SockOptFunc", CURLOPT_SOCKOPTFUNCTION);
	register_option(Module, "SockOptData", CURLOPT_SOCKOPTDATA);
#ifdef LINUX
	register_option(Module, "OpenSocketFunc", CURLOPT_OPENSOCKETFUNCTION);
	register_option(Module, "OpenSocketData", CURLOPT_OPENSOCKETDATA);
#endif
	register_option(Module, "ProgressFunc", CURLOPT_PROGRESSFUNCTION);
	register_option(Module, "ProgressData", CURLOPT_PROGRESSDATA);
	register_option(Module, "HeaderFunc", CURLOPT_HEADERFUNCTION);
	register_option(Module, "WriteHeader", CURLOPT_WRITEHEADER);
	register_option(Module, "DebugFunc", CURLOPT_DEBUGFUNCTION);
	register_option(Module, "DebugData", CURLOPT_DEBUGDATA);
	register_option(Module, "SslCtxFunc", CURLOPT_SSL_CTX_FUNCTION);
	register_option(Module, "SslCtxData", CURLOPT_SSL_CTX_DATA);
	register_option(Module, "ConvToNetworkFunc", CURLOPT_CONV_TO_NETWORK_FUNCTION);
	register_option(Module, "ConvFromNetworkFunc", CURLOPT_CONV_FROM_NETWORK_FUNCTION);
	register_option(Module, "ConvFromUTF8Func", CURLOPT_CONV_FROM_UTF8_FUNCTION);
	register_option(Module, "ErrorBuffer", CURLOPT_ERRORBUFFER);
	register_option(Module, "Stderr", CURLOPT_STDERR);
	register_option(Module, "FailOnError", CURLOPT_FAILONERROR);
	register_option(Module, "Url", CURLOPT_URL);
	register_option(Module, "Proxy", CURLOPT_PROXY);
	register_option(Module, "ProxyPort", CURLOPT_PROXYPORT);
	register_option(Module, "ProxyType", CURLOPT_PROXYTYPE);
	register_option(Module, "HttpProxyTunnel", CURLOPT_HTTPPROXYTUNNEL);
	register_option(Module, "Interface", CURLOPT_INTERFACE);
	register_option(Module, "LocalPort", CURLOPT_LOCALPORT);
	register_option(Module, "LocalPortRange", CURLOPT_LOCALPORTRANGE);
	register_option(Module, "DnsCacheTimeout", CURLOPT_DNS_CACHE_TIMEOUT);
	register_option(Module, "DnsUseGlobalCache", CURLOPT_DNS_USE_GLOBAL_CACHE);
	register_option(Module, "BufferSize", CURLOPT_BUFFERSIZE);
	register_option(Module, "Port", CURLOPT_PORT);
	register_option(Module, "TcpNoDelay", CURLOPT_TCP_NODELAY);
	register_option(Module, "NetRc", CURLOPT_NETRC);
	register_option(Module, "NetRcFile", CURLOPT_NETRC_FILE);
	register_option(Module, "UserPwd", CURLOPT_USERPWD);
	register_option(Module, "UserName", CURLOPT_USERNAME);
	register_option(Module, "Password", CURLOPT_PASSWORD);
	register_option(Module, "ProxyUserPwd", CURLOPT_PROXYUSERPWD);
	register_option(Module, "HttpAuth", CURLOPT_HTTPAUTH);
	register_option(Module, "ProxyAuth", CURLOPT_PROXYAUTH);
	register_option(Module, "AutoReferer", CURLOPT_AUTOREFERER);
	register_option(Module, "Encoding", CURLOPT_ENCODING);
	register_option(Module, "FollowLocation", CURLOPT_FOLLOWLOCATION);
	register_option(Module, "UnrestrictedAuth", CURLOPT_UNRESTRICTED_AUTH);
	register_option(Module, "MaxRedirs", CURLOPT_MAXREDIRS);
	register_option(Module, "Put", CURLOPT_PUT);
	register_option(Module, "Post", CURLOPT_POST);
	register_option(Module, "PostFields", CURLOPT_POSTFIELDS);
	register_option(Module, "PostFieldSize", CURLOPT_POSTFIELDSIZE);
	register_option(Module, "PostFieldSizeLarge", CURLOPT_POSTFIELDSIZE_LARGE);
#ifdef LINUX
	register_option(Module, "CopyPostFields", CURLOPT_COPYPOSTFIELDS);
#endif
	register_option(Module, "HttpPost", CURLOPT_HTTPPOST);
	register_option(Module, "Referer", CURLOPT_REFERER);
	register_option(Module, "UserAgent", CURLOPT_USERAGENT);
	register_option(Module, "HttpHeader", CURLOPT_HTTPHEADER);
	register_option(Module, "Http200Aliases", CURLOPT_HTTP200ALIASES);
	register_option(Module, "Cookie", CURLOPT_COOKIE);
	register_option(Module, "CookieFile", CURLOPT_COOKIEFILE);
	register_option(Module, "CookieJar", CURLOPT_COOKIEJAR);
	register_option(Module, "CookieSession", CURLOPT_COOKIESESSION);
	register_option(Module, "CookieList", CURLOPT_COOKIELIST);
	register_option(Module, "HttpGet", CURLOPT_HTTPGET);
	register_option(Module, "HttpVersion", CURLOPT_HTTP_VERSION);
	register_option(Module, "IgnoreContentLength", CURLOPT_IGNORE_CONTENT_LENGTH);
	register_option(Module, "HttpContentDecoding", CURLOPT_HTTP_CONTENT_DECODING);
	register_option(Module, "HttpTransferDecoding", CURLOPT_HTTP_TRANSFER_DECODING);
	register_option(Module, "FtpPort", CURLOPT_FTPPORT);
	register_option(Module, "Quote", CURLOPT_QUOTE);
	register_option(Module, "PostQuote", CURLOPT_POSTQUOTE);
	register_option(Module, "PreQuote", CURLOPT_PREQUOTE);
#ifdef LINUX
	register_option(Module, "DirListOnly", CURLOPT_DIRLISTONLY);
	register_option(Module, "Append", CURLOPT_APPEND);
#endif
	register_option(Module, "FtpUseEprt", CURLOPT_FTP_USE_EPRT);
	register_option(Module, "FtpUseEpsv", CURLOPT_FTP_USE_EPSV);
	register_option(Module, "FtpCreateMissingDirs", CURLOPT_FTP_CREATE_MISSING_DIRS);
	register_option(Module, "FtpResponseTimeout", CURLOPT_FTP_RESPONSE_TIMEOUT);
	register_option(Module, "FtpAlternativeToUser", CURLOPT_FTP_ALTERNATIVE_TO_USER);
	register_option(Module, "FtpSkipPasvIp", CURLOPT_FTP_SKIP_PASV_IP);
#ifdef LINUX
	register_option(Module, "UseSsl", CURLOPT_USE_SSL);
#endif
	register_option(Module, "FtpSslAuth", CURLOPT_FTPSSLAUTH);
	register_option(Module, "FtpSslCcc", CURLOPT_FTP_SSL_CCC);
	register_option(Module, "FtpAccount", CURLOPT_FTP_ACCOUNT);
	register_option(Module, "FtpFileMethod", CURLOPT_FTP_FILEMETHOD);
	register_option(Module, "TransferText", CURLOPT_TRANSFERTEXT);
#ifdef LINUX
	register_option(Module, "ProxyTransferMode", CURLOPT_PROXY_TRANSFER_MODE);
#endif
	register_option(Module, "CrLf", CURLOPT_CRLF);
	register_option(Module, "Range", CURLOPT_RANGE);
	register_option(Module, "ResumeFrom", CURLOPT_RESUME_FROM);
	register_option(Module, "ResumeFromLarge", CURLOPT_RESUME_FROM_LARGE);
	register_option(Module, "CustomRequest", CURLOPT_CUSTOMREQUEST);
	register_option(Module, "FileTime", CURLOPT_FILETIME);
	register_option(Module, "NoBody", CURLOPT_NOBODY);
	register_option(Module, "InFileSize", CURLOPT_INFILESIZE);
	register_option(Module, "InFileSizeLarge", CURLOPT_INFILESIZE_LARGE);
	register_option(Module, "Upload", CURLOPT_UPLOAD);
	register_option(Module, "MaxFileSize", CURLOPT_MAXFILESIZE);
	register_option(Module, "MaxFileSizeLarge", CURLOPT_MAXFILESIZE_LARGE);
	register_option(Module, "TimeCondition", CURLOPT_TIMECONDITION);
	register_option(Module, "TimeValue", CURLOPT_TIMEVALUE);
	register_option(Module, "TimeOut", CURLOPT_TIMEOUT);
	register_option(Module, "TimeOutMs", CURLOPT_TIMEOUT_MS);
	register_option(Module, "LowSpeedLimit", CURLOPT_LOW_SPEED_LIMIT);
	register_option(Module, "LowSpeedTime", CURLOPT_LOW_SPEED_TIME);
	register_option(Module, "MaxSendSpeedLarge", CURLOPT_MAX_SEND_SPEED_LARGE);
	register_option(Module, "MaxRecvSpeedLarge", CURLOPT_MAX_RECV_SPEED_LARGE);
	register_option(Module, "MaxConnects", CURLOPT_MAXCONNECTS);
	register_option(Module, "ClosePolicy", CURLOPT_CLOSEPOLICY);
	register_option(Module, "FreshConnect", CURLOPT_FRESH_CONNECT);
	register_option(Module, "ForbidReuse", CURLOPT_FORBID_REUSE);
	register_option(Module, "ConnectTimeout", CURLOPT_CONNECTTIMEOUT);
	register_option(Module, "ConnectTimeoutMs", CURLOPT_CONNECTTIMEOUT_MS);
	register_option(Module, "IpResolve", CURLOPT_IPRESOLVE);
	register_option(Module, "ConnectOnly", CURLOPT_CONNECT_ONLY);
	register_option(Module, "SslCert", CURLOPT_SSLCERT);
	register_option(Module, "SslCertType", CURLOPT_SSLCERTTYPE);
	register_option(Module, "SslKey", CURLOPT_SSLKEY);
	register_option(Module, "SslKeyType", CURLOPT_SSLKEYTYPE);
#ifdef LINUX
	register_option(Module, "KeyPasswd", CURLOPT_KEYPASSWD);
#endif
	register_option(Module, "SslEngine", CURLOPT_SSLENGINE);
	register_option(Module, "SslEngineDefault", CURLOPT_SSLENGINE_DEFAULT);
	register_option(Module, "SslVersion", CURLOPT_SSLVERSION);
	register_option(Module, "SslVerifyPeer", CURLOPT_SSL_VERIFYPEER);
	register_option(Module, "CaInfo", CURLOPT_CAINFO);
	register_option(Module, "CaPath", CURLOPT_CAPATH);
	register_option(Module, "RandomFile", CURLOPT_RANDOM_FILE);
	register_option(Module, "EgdSocket", CURLOPT_EGDSOCKET);
	register_option(Module, "SslVerifyHost", CURLOPT_SSL_VERIFYHOST);
	register_option(Module, "SslCipherList", CURLOPT_SSL_CIPHER_LIST);
	register_option(Module, "SslSessionIdCache", CURLOPT_SSL_SESSIONID_CACHE);
#ifdef LINUX
	register_option(Module, "KrbLevel", CURLOPT_KRBLEVEL);
#endif
	register_option(Module, "SshAuthTypes", CURLOPT_SSH_AUTH_TYPES);
	register_option(Module, "SshKnownHosts", CURLOPT_SSH_KNOWNHOSTS);
	register_option(Module, "SshKeyFunc", CURLOPT_SSH_KEYFUNCTION);
#ifdef LINUX
	register_option(Module, "SshHostPublicKeyMd5", CURLOPT_SSH_HOST_PUBLIC_KEY_MD5);
#endif
	register_option(Module, "SshPublicKeyFile", CURLOPT_SSH_PUBLIC_KEYFILE);
	register_option(Module, "SshPrivateKeyFile", CURLOPT_SSH_PRIVATE_KEYFILE);
	register_option(Module, "Private", CURLOPT_PRIVATE);
	register_option(Module, "Share", CURLOPT_SHARE);
#ifdef LINUX
	register_option(Module, "NewFilePerms", CURLOPT_NEW_FILE_PERMS);
#endif
	register_option(Module, "TelnetOptions", CURLOPT_TELNETOPTIONS);
	register_option(Module, "MailFrom", CURLOPT_MAIL_FROM);
	register_option(Module, "MailRcpt", CURLOPT_MAIL_RCPT);
	
	register_fake_option(Module, "WriteStream", WriteStreamOption);
	register_fake_option(Module, "ReadStream", ReadStreamOption);
	return (Std$Object$t *)Module;
}

Std$Integer$smallt UseSslNone[1] = {{Std$Integer$SmallT, CURLUSESSL_NONE}};
Std$Integer$smallt UseSslTry[1] = {{Std$Integer$SmallT, CURLUSESSL_TRY}};
Std$Integer$smallt UseSslControl[1] = {{Std$Integer$SmallT, CURLUSESSL_CONTROL}};
Std$Integer$smallt UseSslAll[1] = {{Std$Integer$SmallT, CURLUSESSL_ALL}};

static inline void register_info(Sys$Module$t *Module, char *Name, CURLINFO Value) {
	curl_info_t *Info = new(curl_info_t);
	Info->Type = InfoT;
	Info->Value = Value;
	Sys$Module$export(Module, Name, 0, Info);
}

CONSTANT(Info, Sys$Module$T) {
	Sys$Module$t *Module = Sys$Module$new("Info");
	Sys$Module$export(Module, "T", 0, OptionT);
//	register_info(Module, "", CURLINFO_);
	register_info(Module, "EffectiveUrl", CURLINFO_EFFECTIVE_URL);
	register_info(Module, "ResponseCode", CURLINFO_RESPONSE_CODE);
	register_info(Module, "HttpConnectCode", CURLINFO_HTTP_CONNECTCODE);
	register_info(Module, "FileTime", CURLINFO_FILETIME);
	
	return (Std$Object$t *)Module;
}

GLOBAL_FUNCTION(ShareNew, 0) {
	share_t *Share = new(share_t);
	Share->Type = ShareT;
	Share->Handle = curl_share_init();
	RETURN(Share);
}

GLOBAL_FUNCTION(New, 0) {
	curl_t *Curl = new(curl_t);
	Curl->Type = T;
	Curl->Handle = curl_easy_init();
	curl_easy_setopt(Curl->Handle, CURLOPT_NOSIGNAL, 1);
	RETURN(Curl);
}

GLOBAL_METHOD(Escape, 2, "escape", TYP, T, TYP, Std$String$T) {
	Result->Val = Std$String$new(curl_easy_escape(
		((curl_t *)Args[0].Val)->Handle,
		Std$String$flatten(Args[1].Val),
		((Std$String$t *)Args[1].Val)->Length.Value
	));
	return SUCCESS;
}

GLOBAL_METHOD(Close, 1, "close", TYP, T) {
	curl_easy_cleanup(((curl_t *)Args[0].Val)->Handle);
	return SUCCESS;
}

GLOBAL_METHOD(Dup, 1, "dup", TYP, T) {
	curl_t *Curl = new(curl_t);
	Curl->Type = T;
	Curl->Handle = curl_easy_duphandle(((curl_t *)Args[0].Val)->Handle);
	RETURN(Curl);
}

GLOBAL_METHOD(Get, 2, "get", TYP, T, TYP, InfoT) {
	curl_t *Curl = (curl_t *)Args[0].Val;
	curl_info_t *Info = (curl_info_t *)Args[1].Val;
	switch (Info->Value) {
	case CURLINFO_EFFECTIVE_URL:
	case CURLINFO_FTP_ENTRY_PATH: {
		char *String;
		if (curl_easy_getinfo(Curl->Handle, Info->Value, &String) == CURLE_OK) {
			RETURN(Std$String$copy(String));
		} else {
			FAIL;
		}
	}
	case CURLINFO_HTTP_CONNECTCODE:
	case CURLINFO_RESPONSE_CODE: {
		long Int;
		if (curl_easy_getinfo(Curl->Handle, Info->Value, &Int) == CURLE_OK) {
			RETURN(Std$Integer$new_small(Int));
		} else {
			FAIL;
		}
	}
	case CURLINFO_FILETIME: {
		long Time;
		if (curl_easy_getinfo(Curl->Handle, Info->Value, &Time) == CURLE_OK) {
			RETURN((Std$Object$t *)Sys$Time$new(Time));
		} else {
			FAIL;
		}
	}
	}
	FAIL;
}

static size_t write_stream_function(void *Buffer, size_t Size, size_t Count, IO$Stream$t *Stream) {
	return IO$Stream$write(Stream, Buffer, Size * Count, 1);
}

static size_t read_stream_function(void *Buffer, size_t Size, size_t Count, IO$Stream$t *Stream) {
	return IO$Stream$read(Stream, Buffer, Size * Count, 1);
}

static size_t readwrite_function(void *Buffer, size_t Size, size_t Count, Std$Object$t *Func) {
	Std$Function$result Result;
	Std$Function$status Status = Std$Function$call(Func, 2, &Result, Std$Address$new(Buffer, Size * Count), 0, Std$Integer$new_small(Size * Count), 0);
	if (Status == MESSAGE) return CURL_READFUNC_ABORT;
	return ((Std$Integer$smallt *)Result.Val)->Value;
}

static int xferinfo_function(Std$Object$t *Func, curl_off_t DLTotal, curl_off_t DLNow, curl_off_t ULTotal, curl_off_t ULNow) {
	Std$Function$result Result;
	Std$Function$status Status = Std$Function$call(Func, 4, &Result,
		Std$Integer$new_small(DLTotal), 0,
		Std$Integer$new_small(DLNow), 0,
		Std$Integer$new_small(ULTotal), 0,
		Std$Integer$new_small(ULNow), 0
	);
	return Std$Integer$get_small(Result.Val);
}

Std$Integer$smallt KeyMatchOk[1] = {{Std$Integer$SmallT, CURLKHMATCH_OK}};
Std$Integer$smallt KeyMatchMismatch[1] = {{Std$Integer$SmallT, CURLKHMATCH_MISMATCH}};
Std$Integer$smallt KeyMatchMissing[1] = {{Std$Integer$SmallT, CURLKHMATCH_MISSING}};

Std$Integer$smallt KeyStateFineAddToFile[1] = {{Std$Integer$SmallT, CURLKHSTAT_FINE_ADD_TO_FILE}};
Std$Integer$smallt KeyStateFine[1] = {{Std$Integer$SmallT, CURLKHSTAT_FINE}};
Std$Integer$smallt KeyStateReject[1] = {{Std$Integer$SmallT, CURLKHSTAT_REJECT}};
Std$Integer$smallt KeyStateDefer[1] = {{Std$Integer$SmallT, CURLKHSTAT_DEFER}};

Std$Integer$smallt KeyTypeUnknown[1] = {{Std$Integer$SmallT, CURLKHTYPE_UNKNOWN}};
Std$Integer$smallt KeyTypeRSA1[1] = {{Std$Integer$SmallT, CURLKHTYPE_RSA1}};
Std$Integer$smallt KeyTypeRSA[1] = {{Std$Integer$SmallT, CURLKHTYPE_RSA}};
Std$Integer$smallt KeyTypeDSS[1] = {{Std$Integer$SmallT, CURLKHTYPE_DSS}};
Std$Integer$smallt KeyTypeECDSA[1] = {{Std$Integer$SmallT, CURLKHTYPE_ECDSA}};
Std$Integer$smallt KeyTypeED25519[1] = {{Std$Integer$SmallT, CURLKHTYPE_ED25519}};

static enum curl_khstat ssh_key_function(CURL *Curl, const struct curl_khkey *KnownKey, const struct curl_khkey *FoundKey, enum curl_khmatch Match, Std$Object$t *Func) {
	Std$Function$result Result;
	Std$Function$status Status = Std$Function$call(Func, 5, &Result,
		Std$Integer$new_small(Match), 0,
		KnownKey->len ? Std$String$new_length(KnownKey->key, KnownKey->len) : Std$String$new(KnownKey->key), 0,
		Std$Integer$new_small(KnownKey->keytype), 0,
		KnownKey->len ? Std$String$new_length(FoundKey->key, FoundKey->len) : Std$String$new(FoundKey->key), 0,
		Std$Integer$new_small(FoundKey->keytype), 0
	);
	return Std$Integer$get_small(Result.Val);
}

static struct curl_slist *string_list(Agg$List$t *In) {
	struct curl_slist *Out = NULL;
	for (Agg$List$node *Node = In->Head; Node; Node = Node->Next) {
		Out = curl_slist_append(Out, Std$String$flatten(Node->Value));
	}
	return Out;
}

GLOBAL_METHOD(Reset, 1, "reset", TYP, T) {
	curl_t *Curl = (curl_t *)Args[0].Val;
	curl_easy_reset(Curl->Handle);
	curl_easy_setopt(Curl->Handle, CURLOPT_NOSIGNAL, 1);
	RETURN0;
}

GLOBAL_METHOD(Set, 3, "set", TYP, T, TYP, OptionT, ANY) {
	curl_t *Curl = (curl_t *)Args[0].Val;
	for (size_t I = 2; I < Count; I += 2) {
		curl_option_t *Option = (curl_option_t *)Args[I - 1].Val;
		Std$Object$t *Param = Args[I].Val;
		if (Param->Type == Std$Integer$SmallT) {
			curl_easy_setopt(Curl->Handle, Option->Value, ((Std$Integer$smallt *)Param)->Value);
		} else if (Param->Type == Std$Integer$BigT) {
			curl_easy_setopt(Curl->Handle, Option->Value, Std$Integer$get_s64(Param));
		} else if (Param->Type == Std$String$T) {
			curl_easy_setopt(Curl->Handle, Option->Value, Std$String$flatten(Param));
		} else if (Param->Type == Agg$List$T) {
			curl_easy_setopt(Curl->Handle, Option->Value, string_list((Agg$List$t *)Param));
		} else if (Param->Type == ShareT) {
			curl_easy_setopt(Curl->Handle, Option->Value, ((share_t *)Param)->Handle);
		} else if (Param == Std$Object$Nil) {
			curl_easy_setopt(Curl->Handle, Option->Value, 0);
		} else if (Param == $true) {
			curl_easy_setopt(Curl->Handle, Option->Value, 1);
		} else if (Param == $false) {
			curl_easy_setopt(Curl->Handle, Option->Value, 0);
		} else switch (Option->Value) {
		case FAKEOPT_WRITE_STREAM:
			curl_easy_setopt(Curl->Handle, CURLOPT_WRITEFUNCTION, write_stream_function);
			curl_easy_setopt(Curl->Handle, CURLOPT_WRITEDATA, Param);
			break;
		case FAKEOPT_READ_STREAM:
			curl_easy_setopt(Curl->Handle, CURLOPT_READFUNCTION, read_stream_function);
			curl_easy_setopt(Curl->Handle, CURLOPT_READDATA, Param);
			break;
		case CURLOPT_WRITEFUNCTION:
			curl_easy_setopt(Curl->Handle, CURLOPT_WRITEFUNCTION, readwrite_function);
			curl_easy_setopt(Curl->Handle, CURLOPT_WRITEDATA, Param);
			break;
		case CURLOPT_READFUNCTION:
			curl_easy_setopt(Curl->Handle, CURLOPT_READFUNCTION, readwrite_function);
			curl_easy_setopt(Curl->Handle, CURLOPT_READDATA, Param);
			break;
		case CURLOPT_HEADERFUNCTION:
			curl_easy_setopt(Curl->Handle, CURLOPT_HEADERFUNCTION, readwrite_function);
			curl_easy_setopt(Curl->Handle, CURLOPT_HEADERDATA, Param);
			break;
		case CURLOPT_XFERINFOFUNCTION:
			curl_easy_setopt(Curl->Handle, CURLOPT_XFERINFOFUNCTION, xferinfo_function);
			curl_easy_setopt(Curl->Handle, CURLOPT_XFERINFODATA, Param);
			break;
		case CURLOPT_SSH_KEYFUNCTION:
			curl_easy_setopt(Curl->Handle, CURLOPT_SSH_KEYFUNCTION, ssh_key_function);
			curl_easy_setopt(Curl->Handle, CURLOPT_SSH_KEYDATA, Param);
			break;
		}
	}
	RETURN0;
}

GLOBAL_METHOD(Perform, 1, "perform", TYP, T) {
	curl_t *Curl = (curl_t *)Args[0].Val;
	CURLcode Code = curl_easy_perform(Curl->Handle);
	if (Code != CURLE_OK) {
		SEND(Std$String$new(curl_easy_strerror(Code)));
	} else {
		RETURN0;
	}
}

GLOBAL_FUNCTION(MultiNew, 0) {
	multi_t *Multi = new(multi_t);
	Multi->Type = MultiT;
	Multi->Handle = curl_multi_init();
	RETURN(Multi);
}

GLOBAL_METHOD(MultiAdd, 2, "add", TYP, MultiT, TYP, T) {
	multi_t *Multi = (multi_t *)Args[0].Val;
	curl_t *Curl = (curl_t *)Args[1].Val;
	curl_multi_add_handle(Multi->Handle, Curl->Handle);
	RETURN0;
}

GLOBAL_METHOD(MultiRemove, 2, "remove", TYP, MultiT, TYP, T) {
	multi_t *Multi = (multi_t *)Args[0].Val;
	curl_t *Curl = (curl_t *)Args[1].Val;
	curl_multi_remove_handle(Multi->Handle, Curl->Handle);
	RETURN0;
}

GLOBAL_METHOD(MultiPerform, 1, "perform", TYP, MultiT) {
	multi_t *Multi = (multi_t *)Args[0].Val;
	int Running;
	while (curl_multi_perform(Multi->Handle, &Running) == CURLM_CALL_MULTI_PERFORM);
	RETURN(Std$Integer$new_small(Running));
}

GLOBAL_METHOD(MultiWait, 2, "wait", TYP, MultiT, TYP, Std$Integer$SmallT) {
	multi_t *Multi = (multi_t *)Args[0].Val;
	int Timeout = Std$Integer$get_small(Args[1].Val);
	int NumFds;
	curl_multi_wait(Multi->Handle, 0, 0, Timeout, &NumFds);
	if (NumFds) {
		RETURN(Std$Integer$new_small(NumFds));
	} else {
		FAIL;
	}
}

GLOBAL_METHOD(MultiRead, 1, "read", TYP, MultiT) {
	multi_t *Multi = (multi_t *)Args[0].Val;
	int Msgs;
	CURLMsg *Msg = curl_multi_info_read(Multi->Handle, &Msgs);
	if (Msg == 0) return FAILURE;
	if (Msg->msg == CURLMSG_DONE) {
		curl_t *Curl = new(curl_t);
		Curl->Type = T;
		Curl->Handle = Msg->easy_handle;
		RETURN(Curl);
	}
	FAIL;
}

static void free(void *Ptr) {}

INITIAL() {
	curl_global_init_mem(CURL_GLOBAL_ALL,
		(void *)Riva$Memory$alloc,
		(void *)Riva$Memory$free,
		(void *)Riva$Memory$realloc,
		(void *)Riva$Memory$strdup,
		(void *)Riva$Memory$calloc
	);
}

