#include <Std.h>
#include <Riva/Memory.h>
#include <IO/Stream.h>
#include <lmdb.h>

typedef struct environment_t {
	const Std$Type$t *Type;
	MDB_env *Handle;
} enviroment_t;

TYPE(EnvironmentT);

typedef struct transaction_t {
	const Std$Type$t *Type;
	MDB_txn *Handle;
} transaction_t;

TYPE(TransactionT);

typedef struct database_t {
	const Std$Type$t *Type;
	MDB_dbi Handle;
} database_t;

TYPE(DatabaseT);

Std$Integer$smallt FlagFixedMap[1] = {{Std$Integer$SmallT, MDB_FIXEDMAP}};
Std$Integer$smallt FlagNoSubDir[1] = {{Std$Integer$SmallT, MDB_NOSUBDIR}};
Std$Integer$smallt FlagReadOnly[1] = {{Std$Integer$SmallT, MDB_RDONLY}};
Std$Integer$smallt FlagWriteMap[1] = {{Std$Integer$SmallT, MDB_WRITEMAP}};
Std$Integer$smallt FlagNoMetaSync[1] = {{Std$Integer$SmallT, MDB_NOMETASYNC}};
Std$Integer$smallt FlagNoSync[1] = {{Std$Integer$SmallT, MDB_NOSYNC}};
Std$Integer$smallt FlagMapAsync[1] = {{Std$Integer$SmallT, MDB_MAPASYNC}};
Std$Integer$smallt FlagNoTLS[1] = {{Std$Integer$SmallT, MDB_NOTLS}};
Std$Integer$smallt FlagNoLock[1] = {{Std$Integer$SmallT, MDB_NOLOCK}};
Std$Integer$smallt FlagNoReadAhead[1] = {{Std$Integer$SmallT, MDB_NORDAHEAD}};
Std$Integer$smallt FlagNoMemInit[1] = {{Std$Integer$SmallT, MDB_NOMEMINIT}};
Std$Integer$smallt FlagReverseKey[1] = {{Std$Integer$SmallT, MDB_REVERSEKEY}};
Std$Integer$smallt FlagDuplicateSort[1] = {{Std$Integer$SmallT, MDB_DUPSORT}};
Std$Integer$smallt FlagIntegerKey[1] = {{Std$Integer$SmallT, MDB_INTEGERKEY}};
Std$Integer$smallt FlagDuplicateFixed[1] = {{Std$Integer$SmallT, MDB_DUPFIXED}};
Std$Integer$smallt FlagIntegerDuplicate[1] = {{Std$Integer$SmallT, MDB_INTEGERDUP}};
Std$Integer$smallt FlagReverseDuplicate[1] = {{Std$Integer$SmallT, MDB_REVERSEDUP}};
Std$Integer$smallt FlagCreate[1] = {{Std$Integer$SmallT, MDB_CREATE}};
Std$Integer$smallt FlagNoDuplicateData[1] = {{Std$Integer$SmallT, MDB_NODUPDATA}};
Std$Integer$smallt FlagNoOverwrite[1] = {{Std$Integer$SmallT, MDB_NOOVERWRITE}};
Std$Integer$smallt FlagReserve[1] = {{Std$Integer$SmallT, MDB_RESERVE}};
Std$Integer$smallt FlagAppend[1] = {{Std$Integer$SmallT, MDB_APPEND}};
Std$Integer$smallt FlagAppendDuplicate[1] = {{Std$Integer$SmallT, MDB_APPENDDUP}};
//Std$Integer$smallt Flag[1] = {{Std$Integer$SmallT, MDB_}};

GLOBAL_FUNCTION(New, 0) {
	enviroment_t *Env = new(enviroment_t);
	Env->Type = EnvironmentT;
	int Error = mdb_env_create(&Env->Handle);
	if (Error) {
		Result->Val = Std$String$new("Error creating environment");
		return MESSAGE;
	};
	mdb_env_set_userctx(Env->Handle, Env);
	Result->Val = (Std$Object$t *)Env;
	return SUCCESS;
};

GLOBAL_METHOD(Open, 2, "open", TYP, EnvironmentT, TYP, Std$String$T, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	enviroment_t *Env = (enviroment_t *)Args[0].Val;
	const char *Path = Std$String$flatten(Args[1].Val);
	int Flags = Std$Integer$get_small(Args[2].Val);
	int Mode = Std$Integer$get_small(Args[3].Val);
	int Error = mdb_env_open(Env->Handle, Path, Flags, Mode);
	if (Error) {
		return MESSAGE;
	};
	Result->Arg = Args[0];
	return SUCCESS;
};

GLOBAL_METHOD(Close, 1, "close", TYP, EnvironmentT) {
	
};

GLOBAL_METHOD(Begin, 2, "begin", TYP, EnvironmentT, TYP, Std$Integer$SmallT) {
	enviroment_t *Env = (enviroment_t *)Args[0].Val;
	int Flags = Std$Integer$get_small(Args[1].Val);
	transaction_t *Txn = new (transaction_t);
	Txn->Type = TransactionT;
	int Error = mdb_txn_begin(Env->Handle, 0, Flags, &Txn->Handle);
	if (Error) {
		Result->Val = Std$String$new("Error beginning transaction");
		return MESSAGE;
	};
	Result->Val = (Std$Object$t *)Txn;
	return SUCCESS;
};

GLOBAL_METHOD(Commit, 1, "commit", TYP, TransactionT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	int Error = mdb_txn_commit(Txn->Handle);
	if (Error) {
		Result->Val = Std$String$new("Error committing transaction");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(Abort, 1, "abort", TYP, TransactionT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	mdb_txn_abort(Txn->Handle);
	return SUCCESS;
};

GLOBAL_METHOD(Reset, 1, "reset", TYP, TransactionT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	mdb_txn_reset(Txn->Handle);
	return SUCCESS;
};

GLOBAL_METHOD(Renew, 1, "renew", TYP, TransactionT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	int Error = mdb_txn_renew(Txn->Handle);
	if (Error) {
		Result->Val = Std$String$new("Error committing transaction");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(OpenDatabase, 3, "open", TYP, TransactionT, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	const char *Name = Std$String$flatten(Args[1].Val);
	int Flags = Std$Integer$get_small(Args[2].Val);
	database_t *DB = new(database_t);
	DB->Type = DatabaseT;
	int Error = mdb_dbi_open(Txn->Handle, Name, Flags, &DB->Handle);
	if (Error) {
		Result->Val = Std$String$new("Error opening database");
		return MESSAGE;
	};
	Result->Val = (Std$Object$t *)DB;
	return SUCCESS;
};

METHOD("open", TYP, TransactionT, TYP, Std$Integer$SmallT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	int Flags = Std$Integer$get_small(Args[1].Val);
	database_t *DB = new(database_t);
	DB->Type = DatabaseT;
	int Error = mdb_dbi_open(Txn->Handle, 0, Flags, &DB->Handle);
	if (Error) {
		Result->Val = Std$String$new("Error opening database");
		return MESSAGE;
	};
	Result->Val = (Std$Object$t *)DB;
	return SUCCESS;
};

GLOBAL_METHOD(DropDatabase, 2, "drop", TYP, TransactionT, TYP, DatabaseT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	database_t *DB = (database_t *)Args[1].Val;
	int Error = mdb_drop(Txn->Handle, DB->Handle, 1);
	if (Error) {
		Result->Val = Std$String$new("Error dropping database");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(EmptyDatabase, 2, "empty", TYP, TransactionT, TYP, DatabaseT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	database_t *DB = (database_t *)Args[1].Val;
	int Error = mdb_drop(Txn->Handle, DB->Handle, 0);
	if (Error) {
		Result->Val = Std$String$new("Error emptying database");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(GetValue, 3, "get", TYP, TransactionT, TYP, DatabaseT, TYP, Std$String$T) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	database_t *DB = (database_t *)Args[1].Val;
	MDB_val Key[1];
	MDB_val Value[1];
	Key->mv_size = Std$String$get_length(Args[2].Val);
	Key->mv_data = Std$String$flatten(Args[2].Val);
	int Error = mdb_get(Txn->Handle, DB->Handle, Key, Value);
	if (Error) {
		Result->Val = Std$String$new("Error getting value");
		return MESSAGE;
	};
	Result->Val = Std$String$copy_length(Value->mv_data, Value->mv_size);
	return SUCCESS;
};

METHOD("get", TYP, TransactionT, TYP, DatabaseT, TYP, Std$String$T, TYP, IO$Stream$WriterT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	database_t *DB = (database_t *)Args[1].Val;
	MDB_val Key[1];
	MDB_val Value[1];
	Key->mv_size = Std$String$get_length(Args[2].Val);
	Key->mv_data = Std$String$flatten(Args[2].Val);
	int Error = mdb_get(Txn->Handle, DB->Handle, Key, Value);
	if (Error == MDB_NOTFOUND) {
		return FAILURE;
	};
	if (Error) {
		Result->Val = Std$String$new("Error getting value");
		return MESSAGE;
	};
	IO$Stream$write(Args[3].Val, Value->mv_data, Value->mv_size, 1);
	Result->Arg = Args[3];
	return SUCCESS;
};

GLOBAL_METHOD(PutValue, 4, "put", TYP, TransactionT, TYP, DatabaseT, TYP, Std$String$T, TYP, Std$String$T, TYP, Std$Integer$SmallT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	database_t *DB = (database_t *)Args[1].Val;
	MDB_val Key[1];
	MDB_val Value[1];
	Key->mv_size = Std$String$get_length(Args[2].Val);
	Key->mv_data = Std$String$flatten(Args[2].Val);
	Value->mv_size = Std$String$get_length(Args[3].Val);
	Value->mv_data = Std$String$flatten(Args[3].Val);
	int Flags = Std$Integer$get_small(Args[4].Val);
	int Error = mdb_put(Txn->Handle, DB->Handle, Key, Value, Flags);
	if (Error) {
		Result->Val = Std$String$new("Error setting value");
		return MESSAGE;
	};
	Result->Arg = Args[3];
	return SUCCESS;
};

METHOD("put", TYP, TransactionT, TYP, DatabaseT, TYP, Std$String$T, TYP, IO$Stream$SeekerT, TYP, Std$Integer$SmallT) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	database_t *DB = (database_t *)Args[1].Val;
	MDB_val Key[1];
	MDB_val Value[1];
	Key->mv_size = Std$String$get_length(Args[2].Val);
	Key->mv_data = Std$String$flatten(Args[2].Val);
	Value->mv_size = IO$Stream$remaining(Args[3].Val);
	Value->mv_data = Riva$Memory$alloc_atomic(Value->mv_size);
	IO$Stream$read(Args[3].Val, Value->mv_data, Value->mv_size, 1);
	int Flags = Std$Integer$get_small(Args[4].Val);
	int Error = mdb_put(Txn->Handle, DB->Handle, Key, Value, Flags);
	if (Error) {
		Result->Val = Std$String$new("Error setting value");
		return MESSAGE;
	};
	Result->Arg = Args[3];
	return SUCCESS;
};

GLOBAL_METHOD(DeleteValue, 3, "delete", TYP, TransactionT, TYP, DatabaseT, TYP, Std$String$T) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	database_t *DB = (database_t *)Args[1].Val;
	MDB_val Key[1];
	MDB_val Value[1] = {0, 0};
	Key->mv_size = Std$String$get_length(Args[2].Val);
	Key->mv_data = Std$String$flatten(Args[2].Val);
	int Error = mdb_del(Txn->Handle, DB->Handle, Key, Value);
	if (Error) {
		Result->Val = Std$String$new("Error getting value");
		return MESSAGE;
	};
	return SUCCESS;
};

GLOBAL_METHOD(DeleteDuplicate, 4, "delete", TYP, TransactionT, TYP, DatabaseT, TYP, Std$String$T, TYP, Std$String$T) {
	transaction_t *Txn = (transaction_t *)Args[0].Val;
	database_t *DB = (database_t *)Args[1].Val;
	MDB_val Key[1];
	MDB_val Value[1];
	Key->mv_size = Std$String$get_length(Args[2].Val);
	Key->mv_data = Std$String$flatten(Args[2].Val);
	Value->mv_size = Std$String$get_length(Args[3].Val);
	Value->mv_data = Std$String$flatten(Args[3].Val);
	int Error = mdb_del(Txn->Handle, DB->Handle, Key, Value);
	if (Error) {
		Result->Val = Std$String$new("Error getting value");
		return MESSAGE;
	};
	Result->Val = Std$String$copy_length(Value->mv_data, Value->mv_size);
	return SUCCESS;
};

