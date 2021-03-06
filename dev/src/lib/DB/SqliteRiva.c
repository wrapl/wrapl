#include <Riva.h>
#include <Std.h>
#include <Agg.h>

#include <stdint.h>
//#include "Sqlite3.h"
#include <sqlite3.h>

Std$Integer$smallt OK[] = {{Std$Integer$SmallT, SQLITE_OK}};
Std$Integer$smallt ERROR[] = {{Std$Integer$SmallT, SQLITE_ERROR}};
Std$Integer$smallt INTERNAL[] = {{Std$Integer$SmallT, SQLITE_INTERNAL}};
Std$Integer$smallt PERM[] = {{Std$Integer$SmallT, SQLITE_PERM}};
Std$Integer$smallt ABORT[] = {{Std$Integer$SmallT, SQLITE_ABORT}};
Std$Integer$smallt BUSY[] = {{Std$Integer$SmallT, SQLITE_BUSY}};
Std$Integer$smallt LOCKED[] = {{Std$Integer$SmallT, SQLITE_LOCKED}};
Std$Integer$smallt NOMEM[] = {{Std$Integer$SmallT, SQLITE_NOMEM}};
Std$Integer$smallt READONLY[] = {{Std$Integer$SmallT, SQLITE_READONLY}};
Std$Integer$smallt INTERRUPT[] = {{Std$Integer$SmallT, SQLITE_INTERRUPT}};
Std$Integer$smallt IOERR[] = {{Std$Integer$SmallT, SQLITE_IOERR}};
Std$Integer$smallt CORRUPT[] = {{Std$Integer$SmallT, SQLITE_CORRUPT}};
Std$Integer$smallt NOT_FOUND[] = {{Std$Integer$SmallT, SQLITE_NOTFOUND}};
Std$Integer$smallt FULL[] = {{Std$Integer$SmallT, SQLITE_FULL}};
Std$Integer$smallt CANTOPEN[] = {{Std$Integer$SmallT, SQLITE_CANTOPEN}};
Std$Integer$smallt PROTOCOL[] = {{Std$Integer$SmallT, SQLITE_PROTOCOL}};
Std$Integer$smallt EMPTY[] = {{Std$Integer$SmallT, SQLITE_EMPTY}};
Std$Integer$smallt SCHEME[] = {{Std$Integer$SmallT, SQLITE_SCHEMA}};
Std$Integer$smallt TOOBIG[] = {{Std$Integer$SmallT, SQLITE_TOOBIG}};
Std$Integer$smallt CONSTRAINT[] = {{Std$Integer$SmallT, SQLITE_CONSTRAINT}};
Std$Integer$smallt MISMATCH[] = {{Std$Integer$SmallT, SQLITE_MISMATCH}};
Std$Integer$smallt MISUSE[] = {{Std$Integer$SmallT, SQLITE_MISUSE}};
Std$Integer$smallt NOLFS[] = {{Std$Integer$SmallT, SQLITE_NOLFS}};
Std$Integer$smallt AUTH[] = {{Std$Integer$SmallT, SQLITE_AUTH}};
Std$Integer$smallt FORMAT[] = {{Std$Integer$SmallT, SQLITE_FORMAT}};
Std$Integer$smallt RANGE[] = {{Std$Integer$SmallT, SQLITE_RANGE}};
Std$Integer$smallt NOTADB[] = {{Std$Integer$SmallT, SQLITE_NOTADB}};
Std$Integer$smallt ROW[] = {{Std$Integer$SmallT, SQLITE_ROW}};
Std$Integer$smallt DONE[] = {{Std$Integer$SmallT, SQLITE_DONE}};

TYPE(T);
TYPE(StatementT);

typedef struct database_t {
	Std$Type$t *Type;
	sqlite3 *Handle;
} database_t;

typedef struct statement_t {
	Std$Type$t *Type;
	sqlite3_stmt *Handle;
	int NoOfFields;
	Std$Object$t *Fields[];
} statement_t;

METHOD("errmsg", TYP, T) {
	database_t *DB = Args[0].Val;
	Result->Val = Std$String$copy(sqlite3_errmsg(DB->Handle));
	return SUCCESS;
}

METHOD("close", TYP, T) {
	database_t *DB = Args[0].Val;
	return (sqlite3_close(DB->Handle) == SQLITE_BUSY) ? FAILURE : SUCCESS;
}

METHOD("last_insert_rowid", TYP, T) {
	database_t *DB = Args[0].Val;
	Result->Val = Std$Integer$new_small(sqlite3_last_insert_rowid(DB->Handle));
	return SUCCESS;
}

METHOD("interrupt", TYP, T) {
	database_t *DB = Args[0].Val;
	return SUCCESS;
}

GLOBAL_FUNCTION(Open, 1) {
	sqlite3 *Handle;
	if (sqlite3_open(Std$String$flatten(Args[0].Val), &Handle) == SQLITE_OK) {
		database_t *DB = new(database_t);
		DB->Type = T;
		DB->Handle = Handle;
		Result->Val = DB;
		return SUCCESS;
	} else {
		Result->Val = Std$String$new(sqlite3_errmsg(Handle));
		sqlite3_close(Handle);
		return MESSAGE;
	}
}

METHOD("prepare", TYP, T, TYP, Std$String$T) {
	database_t *DB = Args[0].Val;
	Std$String$t *Str = Args[1].Val;
	sqlite3_stmt *Handle;
	const char *Tail;
	if (sqlite3_prepare_v2(DB->Handle, Std$String$flatten(Str), Str->Length.Value, &Handle, &Tail) == SQLITE_OK) {
		int NoOfFields = sqlite3_column_count(Handle);
		statement_t *S = Riva$Memory$alloc(sizeof(statement_t) + NoOfFields * sizeof(Std$Object$t *));
		S->Type = StatementT;
		S->Handle = Handle;
		S->NoOfFields = NoOfFields;
		for (int I = 0; I < NoOfFields; ++I) S->Fields[I] = Std$String$copy(sqlite3_column_name(Handle, I));
		RETURN(S);
	} else {
		Result->Val = Std$String$new("Error: Sqlite statement prepare failed");
		return MESSAGE;
	}
}

METHOD("finalize", TYP, StatementT) {
	statement_t *S = Args[0].Val;
	sqlite3_finalize(S->Handle);
	return SUCCESS;
}

METHOD("reset", TYP, StatementT) {
	statement_t *S = Args[0].Val;
	sqlite3_reset(S->Handle);
	RETURN0;
}

METHOD("bind", TYP, StatementT, TYP, Std$Integer$SmallT, TYP, Std$Address$T, TYP, Std$Integer$SmallT) {
	statement_t *S = Args[0].Val;
	sqlite3_bind_blob(S->Handle,
		((Std$Integer$smallt *)Args[1].Val)->Value,
		((Std$Address$t *)Args[2].Val)->Value,
		((Std$Integer$smallt *)Args[3].Val)->Value,
		SQLITE_STATIC
	);
	RETURN0;
}

METHOD("bind", TYP, StatementT, TYP, Std$Integer$SmallT, TYP, Std$Real$T) {
	statement_t *S = Args[0].Val;
	sqlite3_bind_double(S->Handle,
		((Std$Integer$smallt *)Args[1].Val)->Value,
		((Std$Real$t *)Args[2].Val)->Value
	);
	RETURN0;
}

METHOD("bind", TYP, StatementT, TYP, Std$Integer$SmallT, TYP, Std$Integer$SmallT) {
	statement_t *S = Args[0].Val;
	sqlite3_bind_int(S->Handle,
		((Std$Integer$smallt *)Args[1].Val)->Value,
		((Std$Integer$smallt *)Args[2].Val)->Value
	);
	RETURN0;
}

METHOD("bind", TYP, StatementT, TYP, Std$Integer$SmallT, VAL, Std$Object$Nil) {
	statement_t *S = Args[0].Val;
	sqlite3_bind_null(S->Handle,
		((Std$Integer$smallt *)Args[1].Val)->Value
	);
	RETURN0;
}

METHOD("bind", TYP, StatementT, TYP, Std$Integer$SmallT, TYP, Std$String$T) {
	statement_t *S = Args[0].Val;
	sqlite3_bind_text(S->Handle,
		((Std$Integer$smallt *)Args[1].Val)->Value,
		Std$String$flatten(Args[2].Val),
		((Std$String$t *)Args[2].Val)->Length.Value,
		SQLITE_STATIC
	);
	RETURN0;
}

METHOD("step", TYP, StatementT) {
	statement_t *S = Args[0].Val;
	int Code = sqlite3_step(S->Handle);
	switch (Code) {
	case SQLITE_DONE:
		sqlite3_reset(S->Handle);
		FAIL;
	case SQLITE_ROW: RETURN0;
	default:
		SEND(Std$Integer$new_small(Code));
	}
}

METHOD("column_blob", TYP, StatementT, TYP, Std$Integer$SmallT) {
	statement_t *S = Args[0].Val;
	Result->Val = Std$Address$new(sqlite3_column_blob(S->Handle, ((Std$Integer$smallt *)Args[1].Val)->Value));
	return SUCCESS;
}

METHOD("column_bytes", TYP, StatementT, TYP, Std$Integer$SmallT) {
	statement_t *S = Args[0].Val;
	Result->Val = Std$Integer$new_small(sqlite3_column_bytes(S->Handle, ((Std$Integer$smallt *)Args[1].Val)->Value));
	return SUCCESS;
}

METHOD("column_count", TYP, StatementT) {
	statement_t *S = Args[0].Val;
	Result->Val = Std$Integer$new_small(S->NoOfFields);
	return SUCCESS;
}

METHOD("column_double", TYP, StatementT, TYP, Std$Integer$SmallT) {
	statement_t *S = Args[0].Val;
	Result->Val = Std$Real$new(sqlite3_column_double(S->Handle, ((Std$Integer$smallt *)Args[1].Val)->Value));
	return SUCCESS;
}

METHOD("column_int", TYP, StatementT, TYP, Std$Integer$SmallT) {
	statement_t *S = Args[0].Val;
	Result->Val = Std$Integer$new_small(sqlite3_column_int(S->Handle, ((Std$Integer$smallt *)Args[1].Val)->Value));
	return SUCCESS;
}

METHOD("column_name", TYP, StatementT, TYP, Std$Integer$SmallT) {
	statement_t *S = Args[0].Val;
	Result->Val = Std$String$new(S->Fields[((Std$Integer$smallt *)Args[1].Val)->Value]);
	return SUCCESS;
}

METHOD("column_text", TYP, StatementT, TYP, Std$Integer$SmallT) {
	statement_t *S = Args[0].Val;
	const char *Text = sqlite3_column_text(S->Handle, ((Std$Integer$smallt *)Args[1].Val)->Value);
	int Length = sqlite3_column_bytes(S->Handle, ((Std$Integer$smallt *)Args[1].Val)->Value);
	Result->Val = Std$String$copy_length(Text, Length);
	return SUCCESS;
}

METHOD("column_type", TYP, StatementT, TYP, Std$Integer$SmallT) {
	statement_t *S = Args[0].Val;
	Result->Val = Std$Integer$new_small(sqlite3_column_type(S->Handle, ((Std$Integer$smallt *)Args[1].Val)->Value));
	return SUCCESS;
}

typedef struct result_generator {
	Std$Function$cstate State;
	statement_t *Statement;
} result_generator;

static Std$Function$status resume_result_generator(Std$Function$result *Result) {
	result_generator *Generator = Result->State;
	statement_t *S = Generator->Statement;
	sqlite3_stmt *Handle = S->Handle;
	int Code = sqlite3_step(Handle);
	switch (Code) {
	case SQLITE_DONE:
		sqlite3_reset(Handle);
		return FAILURE;
	case SQLITE_ROW: {
		Std$Object$t *Results = Agg$List$new0();
		for (int I = 0; I < S->NoOfFields; ++I) {
			Std$Object$t *Value;
			switch (sqlite3_column_type(S->Handle, I)) {
			case SQLITE_INTEGER:
				Value = Std$Integer$new_small(sqlite3_column_int(Handle, I));
				break;
			case SQLITE_FLOAT:
				Value = Std$Real$new(sqlite3_column_double(Handle, I));
				break;
			case SQLITE_TEXT:
				Value = Std$String$copy_length(sqlite3_column_text(Handle, I), sqlite3_column_bytes(S->Handle, I));
				break;
			case SQLITE_BLOB:
				Value = Std$Address$new(sqlite3_column_blob(Handle, I));
				break;
			case SQLITE_NULL:
				Value = Std$Object$Nil;
				break;
			}
			Agg$List$put(Results, Value);
		}
		Result->Val = Results;
		return SUSPEND;
	}
	default:
		Result->Val = Std$Integer$new_small(Code);
		return MESSAGE;
	}
}

METHOD("()", TYP, StatementT) {
	statement_t *S = Args[0].Val;
	sqlite3_stmt *Handle = S->Handle;
	sqlite3_reset(Handle);
	int I = 1;
	for (int J = 1; J < Count; ++I) {
		Std$Object$t *Arg = Args[J].Val;
		if (Arg->Type == Std$Address$T) {
			CHECK_EXACT_ARG_TYPE(J + 1, Std$Integer$SmallT);
			sqlite3_bind_blob(Handle, I, ((Std$Address$t *)Arg)->Value, ((Std$Integer$smallt *)Args[J + 1].Val)->Value, SQLITE_STATIC);
			J += 2;
		} else if (Arg->Type == Std$Real$T) {
			sqlite3_bind_double(Handle, I, ((Std$Real$t *)Arg)->Value);
			J += 1;
		} else if (Arg->Type == Std$Integer$SmallT) {
			sqlite3_bind_int(Handle, I, ((Std$Integer$smallt *)Arg)->Value);
			J += 1;
		} else if (Arg == Std$Object$Nil) {
			sqlite3_bind_null(Handle, I);
			J += 1;
		} else if (Arg->Type == Std$String$T) {
			sqlite3_bind_text(Handle, I, Std$String$flatten(Arg), ((Std$String$t *)Arg)->Length.Value, SQLITE_STATIC);
			J += 1;
		} else {
			Result->Val = Std$String$new("Invalid bind argument"); return MESSAGE;
		}
	}
	int Code;
	for (;;) switch (Code = sqlite3_step(Handle)) {
	case SQLITE_DONE:
		sqlite3_reset(Handle);
		return FAILURE;
	case SQLITE_ROW: {
		Std$Object$t *Results = Agg$List$new0();
		for (int I = 0; I < S->NoOfFields; ++I) {
			Std$Object$t *Value;
			switch (sqlite3_column_type(S->Handle, I)) {
			case SQLITE_INTEGER:
				Value = Std$Integer$new_small(sqlite3_column_int(Handle, I));
				break;
			case SQLITE_FLOAT:
				Value = Std$Real$new(sqlite3_column_double(Handle, I));
				break;
			case SQLITE_TEXT:
				Value = Std$String$copy_length(sqlite3_column_text(Handle, I), sqlite3_column_bytes(S->Handle, I));
				break;
			case SQLITE_BLOB:
				Value = Std$Address$new(sqlite3_column_blob(Handle, I));
				break;
			case SQLITE_NULL:
				Value = Std$Object$Nil;
				break;
			}
			Agg$List$put(Results, Value);
		}
		result_generator *Generator = new(result_generator);
		Generator->State.Run = Std$Function$resume_c;
		Generator->State.Invoke = resume_result_generator;
		Generator->Statement = S;
		Result->State = Generator;
		Result->Val = Results;
		return SUSPEND;
	}
	default:
		Result->Val = Std$Integer$new_small(Code);
		return MESSAGE;
	}
}

typedef struct callback_t {
    Std$Object$t *Function;
    Std$Function$argument Arg;
} callback_t;

static int exec_callback(callback_t *Callback, int Count, char **Values, char **Keys) {
    Std$Object$t *Table = Agg$Table$new(Std$String$Compare, Std$String$Hash);
    for (int I = 0; I < Count; ++I) {
        Agg$Table$insert(Table, Std$String$copy(Keys[I]), Std$String$copy(Values[I]));
    }
    Std$Function$result Buffer;
    return (Std$Function$call(Callback->Function, 2, &Buffer, Table, 0, Callback->Arg.Val, Callback->Arg.Ref) >= FAILURE);
}

METHOD("exec", TYP, T, TYP, Std$String$T, TYP, Std$Function$T) {
    database_t *DB = Args[0].Val;
    callback_t Callback = {
        Args[2].Val,
        Count > 3 ? Args[3] : (Std$Function$argument){Std$Object$Nil, 0}
    };
    char *ErrMsg;
    if (sqlite3_exec(DB->Handle, Std$String$flatten(Args[1].Val), exec_callback, &Callback, &ErrMsg) == SQLITE_OK) {
        Result->Arg = Args[0];
        return SUCCESS;
    } else {
        Result->Val = Std$String$new(ErrMsg);
        return MESSAGE;
    }
}

METHOD("exec", TYP, T, TYP, Std$String$T) {
    database_t *DB = Args[0].Val;
    char *ErrMsg;
    if (sqlite3_exec(DB->Handle, Std$String$flatten(Args[1].Val), 0, 0, &ErrMsg) == SQLITE_OK) {
        Result->Arg = Args[0];
        return SUCCESS;
    } else {
        Result->Val = Std$String$new(ErrMsg);
        return MESSAGE;
    }
}

static int execv_callback(callback_t *Callback, int Count, char **Values, char **Keys) {
    Std$Function$argument Args[Count + 1];
    for (int I = 0; I < Count; ++I) {
        Args[I].Val = Std$String$copy(Values[I]);
        Args[I].Ref = 0;
    }
    Args[Count] = Callback->Arg;
    Std$Function$result Buffer;
    return (Std$Function$invoke(Callback->Function, Count + 1, &Buffer, Args) >= FAILURE);
}

METHOD("execv", TYP, T, TYP, Std$String$T, TYP, Std$Function$T) {
    database_t *DB = Args[0].Val;
    callback_t Callback = {
        Args[2].Val,
        Count > 3 ? Args[3] : (Std$Function$argument){Std$Object$Nil, 0}
    };
    char *ErrMsg;
    if (sqlite3_exec(DB->Handle, Std$String$flatten(Args[1].Val), execv_callback, &Callback, &ErrMsg) == SQLITE_OK) {
        Result->Arg = Args[0];
        return SUCCESS;
    } else {
        Result->Val = Std$String$new(ErrMsg);
        return MESSAGE;
    }
}

INITIAL() {
	//sqlite3Os.xMalloc = Riva$Memory$alloc;
}
