#include <Std.h>
#include <Riva.h>
#include <Agg/List.h>
#include <Agg/Buffer.h>

#include <pocketsphinx.h>

typedef struct decoder_t {
	const Std$Type$t *Type;
	ps_decoder_t *Handle;
} decoder_t;

typedef struct segment_t {
	const Std$Type$t *Type;
	Std$String$t *Word;
	Std$Integer$smallt *Start;
	Std$Integer$smallt *End;
} segment_t;

TYPE(T);

SYMBOL($word, "word");
SYMBOL($start, "start");
SYMBOL($end, "end");

TYPEF(SegmentT, ($word, $start, $end));

GLOBAL_FUNCTION(New, 1) {
	CHECK_ARG_TYPE(0, Agg$List$T);
	Agg$List$t *Options = (Agg$List$t *)Args[0].Val;
	char **Argv[Options->Length];
	int Index = 0;
	for (Agg$List$node *Node = Options->Head; Node; Node = Node->Next) {
		if (Node->Value->Type != Std$String$T) {
			Result->Val = Std$String$new("Arguments must be strings");
			return MESSAGE;
		}
		Argv[Index] = Std$String$flatten(Node->Value);
		++Index;
	}
	cmd_ln_t *CmdLine = cmd_ln_parse_r(0, ps_args(), Options->Length, Argv, 0);
	if (!CmdLine) {
		Result->Val = Std$String$new("Error parsing arguments");
		return MESSAGE;
	}
	decoder_t *Decoder = new(decoder_t);
	Decoder->Type = T;
	Decoder->Handle = ps_init(CmdLine);
	Result->Val = (Std$Object$t *)Decoder;
	return SUCCESS;
}

METHOD("start_stream", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	if (ps_start_stream(Decoder->Handle) < 0) {
		Result->Val = Std$String$new("Error starting stream");
		return MESSAGE;
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("start_utt", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	if (ps_start_utt(Decoder->Handle) < 0) {
		Result->Val = Std$String$new("Error starting utterance");
		return MESSAGE;
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("end_utt", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	if (ps_end_utt(Decoder->Handle) < 0) {
		Result->Val = Std$String$new("Error ending utterance");
		return MESSAGE;
	}
	Result->Arg = Args[0];
	return SUCCESS;
}

METHOD("process", TYP, T, TYP, Agg$Buffer$Int16$T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	Agg$Buffer$t *Buffer = (Agg$Buffer$t *)Args[1].Val;
	int Processed = ps_process_raw(Decoder->Handle, Buffer->Value, Buffer->Length.Value, 0, 0);
	if (Processed < 0) {
		Result->Val = Std$String$new("Error processing data");
		return MESSAGE;
	}
	Result->Val = Std$Integer$new_small(Processed);
	return SUCCESS;
}

METHOD("hypothesis", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	int BestScore = 0;
	char const *Hypothesis = ps_get_hyp(Decoder->Handle, &BestScore);
	if (!Hypothesis) return FAILURE;
	if (Count > 1 && Args[1].Ref) Args[1].Ref[0] = Std$Integer$new_small(BestScore);
	Result->Val = Std$String$copy(Hypothesis);
	return SUCCESS;
}

METHOD("segments", TYP, T) {
	decoder_t *Decoder = (decoder_t *)Args[0].Val;
	Std$Object$t *Segments = Agg$List$new(0);
	for (ps_seg_t *Seg = ps_seg_iter(Decoder->Handle); Seg; Seg = ps_seg_next(Seg)) {
		segment_t *Segment = new(segment_t);
		Segment->Type = SegmentT;
		Segment->Word = Std$String$copy(ps_seg_word(Seg));
		int Start, End;
		ps_seg_frames(Seg, &Start, &End);
		Segment->Start = Std$Integer$new_small(Start);
		Segment->End = Std$Integer$new_small(End);
		Agg$List$put(Segments, Segment);
	}
	Result->Val = Segments;
	return SUCCESS;
}

METHOD("word", TYP, SegmentT) {
	segment_t *Segment = (segment_t *)Args[0].Val;
	Result->Val = Segment->Word;
	return SUCCESS;
}

METHOD("start", TYP, SegmentT) {
	segment_t *Segment = (segment_t *)Args[0].Val;
	Result->Val = Segment->Start;
	return SUCCESS;
}

METHOD("end", TYP, SegmentT) {
	segment_t *Segment = (segment_t *)Args[0].Val;
	Result->Val = Segment->End;
	return SUCCESS;
}
