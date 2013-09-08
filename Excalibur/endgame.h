#ifndef __endgame_h__
#define __endgame_h__

#include "position.h"

/* Endgame KP vs K table base -- kpkbase.cpp */
namespace KPKbase
{
	// initialized in Endgame::init()
	void init();
	// true = WIN, false = DRAW
	bool probe(Square wksq, Square wpsq, Square bksq, Color us);
}

// EndgameType lists all supported endgames
// There're 2 types of EndEvaluator: evalFunc() and scalingFunc()
enum EndgameType
{
	// Evaluation functions
	KXK,   // Generic "mate lone king" eval; NonUnique
	KNNK,  // KNN vs K
	KBNK,  // KBN vs K
	KPK,   // KP vs K
	KRKP,  // KR vs KP
	KRKB,  // KR vs KB
	KRKN,  // KR vs KN
	KQKP,  // KQ vs KP
	KQKR,  // KQ vs KR
	KBBKN, // KBB vs KN
	KmmKm, // K and two minors vs K and one or two minors; NonUnique

	// Scaling functions
	KBPsK,   // KB+pawns vs K; NonUnique
	KQKRPs,  // KQ vs KR+pawns; NonUnique
	KRPKR,   // KRP vs KR
	KRPPKRP, // KRPP vs KRP
	KPsK,    // K+pawns vs K; NonUnique
	KBPKB,   // KBP vs KB
	KBPPKB,  // KBPP vs KB
	KBPKN,   // KBP vs KN
	KNPK,    // KNP vs K
	KNPKB,   // KNP vs KB
	KPKP     // KP vs KP; NonUnique because the strongerSide color can't be determined
};

// Base template for EndEvaluator (evalFunc and scalingFunc)
class EndEvaluatorBase
{
public:
	virtual Color strong_color() const = 0;  // get the stronger side
	virtual int operator()(const Position&) const = 0; // return either Value or ScaleFactor
};

template<EndgameType Eg>
class EndEvaluator : public EndEvaluatorBase
{
public:
	explicit EndEvaluator(Color c) : strongerSide(c), weakerSide(~c) {}
	Color strong_color() const { return strongerSide; }
	int operator()(const Position&) const; // return either Value or ScaleFactor
private:
	Color strongerSide, weakerSide;
};

namespace Endgame
{
	typedef map<U64, unique_ptr<EndEvaluatorBase>> EgMap;
	// key and evaluator function collection. Init at program startup.
	extern EgMap EvalFuncMap; 
	extern EgMap ScalingFuncMap; 

	typedef map<EndgameType, unique_ptr<EndEvaluatorBase>> EgMap2[COLOR_N];
	// A few material distribution, like KBPsK, cannot be determined 
	// by a unique position material hash key (i.e. corresponds to more than 
	// 1 material configuration) Thus we use the EgType explicitly as the map key.
	extern EgMap2 NonUniqueFuncMap;

	// initialized in Eval::init()
	void init();

	// map[key].get() gets the original pointer from the unique_ptr<>
	inline EndEvaluatorBase* probe_eval_func(U64 key)
	{  return EvalFuncMap.count(key) ? EvalFuncMap[key].get() : nullptr; }

	inline EndEvaluatorBase* probe_scaling_func(U64 key)
	{  return ScalingFuncMap.count(key) ? ScalingFuncMap[key].get() : nullptr; }

	template<EndgameType Eg, Color c>
	inline EndEvaluatorBase* probe_non_unique_func()
	{	return NonUniqueFuncMap[c][Eg].get(); }
}

#endif // __endgame_h__
