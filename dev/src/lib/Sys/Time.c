#include <Std.h>
#include <Sys/Time.h>
#include <Riva/Memory.h>

#include <stdlib.h>
#include <string.h>

TYPE(T);
// A time value with second resolution.

TYPE(PreciseT, T);
// A time value with nanosecond resolution.

GLOBAL_FUNCTION(Now, 0) {
//:T
// The current time.
	Sys$Time_t *Time = new(Sys$Time_t);
	Time->Type = T;
	Time->Value = time(0);
	Result->Val = Time;
	return SUCCESS;
};

GLOBAL_FUNCTION(New, 1) {
	if (Args[0].Val->Type == Std$Integer$SmallT) {
		Sys$Time_t *Time = new(Sys$Time_t);
		Time->Type = T;
		Time->Value = Std$Integer$get_small(Args[0].Val);
		RETURN(Time);
	} else if (Args[0].Val->Type == Std$Real$T) {
		Sys$Time_t *Time = new(Sys$Time_t);
		Time->Type = T;
		Time->Value = Std$Real$get_value(Args[0].Val);
		RETURN(Time);
	} else if (Args[0].Val->Type == Std$String$T) {
		CHECK_EXACT_ARG_TYPE(1, Std$String$T);
		const char *String = Std$String$flatten(Args[0].Val);
		const char *Format = Std$String$flatten(Args[1].Val);
		struct tm Tm[1] = {0,};
		if (!strptime(String, Format, Tm)) {
			SEND(Std$String$new("Error parsing time"));
		}
		Sys$Time_t *Time = new(Sys$Time_t);
		Time->Type = T;
		Time->Value = timegm(Tm);
		RETURN(Time);
	}
	SEND(Std$String$new("Invalid parameter type"));
};

Sys$Time_t *_new(time_t Value) {
	Sys$Time_t *Time = new(Sys$Time_t);
	Time->Type = T;
	Time->Value = Value;
	return Time;
};

METHOD("?", TYP, T, TYP, T) {
	Sys$Time_t *A = Args[0].Val;
	Sys$Time_t *B = Args[1].Val;
	if (A->Value > B->Value) {
		Result->Val = Std$Object$Greater;
	} else if (A->Value < B->Value) {
		Result->Val = Std$Object$Less;
	} else {
		Result->Val = Std$Object$Equal;
	};
	return SUCCESS;
};

/*METHOD(">", TYP, T, TYP, T) {
	Sys$Time_t *A = Args[0].Val;
	Sys$Time_t *B = Args[1].Val;
	if (A->Value > B->Value) {
		Result->Arg = Args[1];
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("<", TYP, T, TYP, T) {
	Sys$Time_t *A = Args[0].Val;
	Sys$Time_t *B = Args[1].Val;
	if (A->Value < B->Value) {
		Result->Arg = Args[1];
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD(">=", TYP, T, TYP, T) {
	Sys$Time_t *A = Args[0].Val;
	Sys$Time_t *B = Args[1].Val;
	if (A->Value >= B->Value) {
		Result->Arg = Args[1];
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("<=", TYP, T, TYP, T) {
	Sys$Time_t *A = Args[0].Val;
	Sys$Time_t *B = Args[1].Val;
	if (A->Value <= B->Value) {
		Result->Arg = Args[1];
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("=", TYP, T, TYP, T) {
	Sys$Time_t *A = Args[0].Val;
	Sys$Time_t *B = Args[1].Val;
	if (A->Value == B->Value) {
		Result->Arg = Args[1];
		return SUCCESS;
	} else {
		return FAILURE;
	};
};

METHOD("~=", TYP, T, TYP, T) {
	Sys$Time_t *A = Args[0].Val;
	Sys$Time_t *B = Args[1].Val;
	if (A->Value != B->Value) {
		Result->Arg = Args[1];
		return SUCCESS;
	} else {
		return FAILURE;
	};
};*/

METHOD("@", TYP, T, VAL, Std$String$T) {
	Sys$Time_t *Time = Args[0].Val;
	/*char *Buffer = Riva$Memory$alloc_atomic(26);
	ctime_r(&Time->Value, Buffer);
	Buffer[strlen(Buffer) - 1] = 0;*/
	struct tm TM[1];
	gmtime_r(&Time->Value, TM);
	char *Buffer = Riva$Memory$alloc_atomic(40);
	int Length = strftime(Buffer, 40, "%F %T%z", TM);
	Result->Val = Std$String$new_length(Buffer, Length);
	return SUCCESS;
};

METHOD("@", TYP, T, VAL, Std$Integer$SmallT) {
	Sys$Time_t *Time = Args[0].Val;
	Result->Val = Std$Integer$new_small(Time->Value);
	return SUCCESS;
};

METHOD("-", TYP, T, TYP, T) {
	Sys$Time_t *A = Args[0].Val;
	Sys$Time_t *B = Args[1].Val;
	Result->Val = Std$Integer$new_small(A->Value - B->Value);
	return SUCCESS;
};

METHOD("-", TYP, T, TYP, Std$Integer$SmallT) {
	Sys$Time_t *A = Args[0].Val;
	Std$Integer_smallt *B = Args[1].Val;
	Result->Val = _new(A->Value - B->Value);
	return SUCCESS;
};

METHOD("+", TYP, T, TYP, Std$Integer$SmallT) {
	Sys$Time_t *A = Args[0].Val;
	Std$Integer_smallt *B = Args[1].Val;
	Result->Val = _new(A->Value + B->Value);
	return SUCCESS;
};

METHOD("+", TYP, Std$Integer$SmallT, TYP, T) {
	Std$Integer_smallt *A = Args[0].Val;
	Sys$Time_t *B = Args[1].Val;
	Result->Val = _new(A->Value + B->Value);
	return SUCCESS;
};

GLOBAL_FUNCTION(PreciseNow, 0) {
//:PreciseT
// The precise current time.
	Sys$Time$precise_t *Time = new(Sys$Time$precise_t);
	Time->Type = PreciseT;
	gettimeofday(&Time->Value, 0);
	Result->Val = Time;
	return SUCCESS;
};

GLOBAL_FUNCTION(PreciseNew, 1) {
	CHECK_ARG_TYPE(0, Std$Integer$T);
	Sys$Time$precise_t *Time = new(Sys$Time$precise_t);
	Time->Type = PreciseT;
	uint64_t Micros = 0;
	if (Args[0].Val->Type == Std$Integer$SmallT) {
		Micros = Std$Integer$get_small(Args[0].Val);
	} else if (Args[0].Val->Type == Std$Integer$BigT) {
		Micros = Std$Integer$get_u64(Args[0].Val);
	};
	Time->Value.tv_sec = Micros / 1000000LL;
	Time->Value.tv_usec = Micros % 1000000LL;
	Result->Val = Time;
	return SUCCESS;
};

METHOD("@", TYP, PreciseT, VAL, Std$String$T) {
	Sys$Time$precise_t *Time = Args[0].Val;
	struct tm TM[1];
	gmtime_r(&Time->Value.tv_sec, TM);
	char *Buffer = Riva$Memory$alloc_atomic(40);
	char *Temp = Buffer;
	Temp += strftime(Temp, 40, "%F %T", TM);
	Temp += sprintf(Temp, ".%06ld", Time->Value.tv_usec);
	Temp += strftime(Temp, 40, "%z", TM);
	int Length = Temp - Buffer;
	Result->Val = Std$String$new_length(Buffer, Length);
	return SUCCESS;
};

Sys$Time$precise_t *_precise_new(uint64_t Micros) {
	Sys$Time$precise_t *Time = new(Sys$Time$precise_t);
	Time->Type = PreciseT;
	Time->Value.tv_sec = Micros / 1000000LL;
	Time->Value.tv_usec = Micros % 1000000LL;
	return Time;
};

METHOD("-", TYP, PreciseT, TYP, PreciseT) {
	Sys$Time$precise_t *A = Args[0].Val;
	int64_t AMicros = A->Value.tv_sec * 1000000LL + A->Value.tv_usec;
	Sys$Time$precise_t *B = Args[1].Val;
	int64_t BMicros = B->Value.tv_sec * 1000000LL + B->Value.tv_usec;
	Result->Val = Std$Integer$new_s64(A - B);
	return SUCCESS;
};

METHOD("-", TYP, PreciseT, TYP, Std$Integer$SmallT) {
	Sys$Time$precise_t *A = Args[0].Val;
	int64_t AMicros = A->Value.tv_sec * 1000000LL + A->Value.tv_usec;
	Std$Integer_smallt *B = Args[1].Val;
	Result->Val = _precise_new(AMicros - B->Value);
	return SUCCESS;
};

METHOD("-", TYP, PreciseT, TYP, Std$Integer$BigT) {
	Sys$Time$precise_t *A = Args[0].Val;
	int64_t AMicros = A->Value.tv_sec * 1000000LL + A->Value.tv_usec;
	Std$Integer_bigt *B = Args[1].Val;
	Result->Val = _precise_new(AMicros - Std$Integer$get_s64(B));
	return SUCCESS;
};

METHOD("+", TYP, PreciseT, TYP, Std$Integer$SmallT) {
	Sys$Time$precise_t *A = Args[0].Val;
	int64_t AMicros = A->Value.tv_sec * 1000000LL + A->Value.tv_usec;
	Std$Integer_smallt *B = Args[1].Val;
	Result->Val = _precise_new(AMicros + B->Value);
	return SUCCESS;
};

METHOD("+", TYP, Std$Integer$SmallT, TYP, PreciseT) {
	Std$Integer_smallt *A = Args[0].Val;
	Sys$Time$precise_t *B = Args[1].Val;
	int64_t BMicros = B->Value.tv_sec * 1000000LL + B->Value.tv_usec;
	Result->Val = _precise_new(A->Value + BMicros);
	return SUCCESS;
};

METHOD("+", TYP, PreciseT, TYP, Std$Integer$BigT) {
	Sys$Time$precise_t *A = Args[0].Val;
	int64_t AMicros = A->Value.tv_sec * 1000000LL + A->Value.tv_usec;
	Std$Integer_bigt *B = Args[1].Val;
	Result->Val = _precise_new(AMicros + Std$Integer$get_s64(B));
	return SUCCESS;
};

METHOD("+", TYP, Std$Integer$BigT, TYP, PreciseT) {
	Std$Integer_bigt *A = Args[0].Val;
	Sys$Time$precise_t *B = Args[1].Val;
	int64_t BMicros = B->Value.tv_sec * 1000000LL + B->Value.tv_usec;
	Result->Val = _precise_new(Std$Integer$get_s64(A) + BMicros);
	return SUCCESS;
};

METHOD("millis", TYP, T) {
	Sys$Time$precise_t *Time = Args[0].Val;
	uint64_t Millis = Time->Value.tv_sec * 1000LL + Time->Value.tv_usec / 1000LL;
	Result->Val = Std$Integer$new_u64(Millis);
	return SUCCESS;
};

METHOD("micros", TYP, T) {
	Sys$Time$precise_t *Time = Args[0].Val;
	uint64_t Micros = Time->Value.tv_sec * 1000000LL + Time->Value.tv_usec;
	Result->Val = Std$Integer$new_u64(Micros);
	return SUCCESS;
};

METHOD("?", TYP, PreciseT, TYP, PreciseT) {
	Sys$Time$precise_t *A = Args[0].Val;
	Sys$Time$precise_t *B = Args[1].Val;
	if (A->Value.tv_sec > B->Value.tv_sec) {
		Result->Val = Std$Object$Greater;
	} else if (A->Value.tv_sec < B->Value.tv_sec) {
		Result->Val = Std$Object$Less;
	} else if (A->Value.tv_usec > B->Value.tv_usec) {
		Result->Val = Std$Object$Greater;
	} else if (A->Value.tv_usec < B->Value.tv_usec) {
		Result->Val = Std$Object$Less;
	} else {
		Result->Val = Std$Object$Equal;
	};
	return SUCCESS;
};

METHOD("sec", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		RETURN(Std$Integer$new_small(LocalTime->tm_sec));
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}

METHOD("min", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		RETURN(Std$Integer$new_small(LocalTime->tm_min));
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}

METHOD("hour", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		RETURN(Std$Integer$new_small(LocalTime->tm_hour));
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}

METHOD("mday", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		RETURN(Std$Integer$new_small(LocalTime->tm_mday));
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}

METHOD("mon", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		RETURN(Std$Integer$new_small(LocalTime->tm_mon));
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}

METHOD("year", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		RETURN(Std$Integer$new_small(LocalTime->tm_year));
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}

METHOD("wday", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		RETURN(Std$Integer$new_small(LocalTime->tm_wday));
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}

METHOD("yday", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		RETURN(Std$Integer$new_small(LocalTime->tm_yday));
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}

METHOD("isdst", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		if (LocalTime->tm_isdst) {
			RETURN0;
		} else {
			FAIL;
		}
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}

METHOD("gmtoff", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		RETURN(Std$Integer$new_small(LocalTime->tm_gmtoff));
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}

METHOD("zone", TYP, T) {
	Sys$Time$t *Time = (Sys$Time$t *)Args[0].Val;
	struct tm LocalTime[1];
	if (gmtime_r(&Time->Value, LocalTime)) {
		RETURN(Std$String$new(LocalTime->tm_zone));
	} else {
		SEND(Std$String$new("Time conversion error"));
	}
}
