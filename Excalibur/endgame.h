#ifndef __endgame_h__
#define __endgame_h__

#include "position.h"

// EndgameType lists all supported endgames
// There're 2 types of EndEvaluator: evalFunc() and scalingFunc()
enum EndgameType {

	// Evaluation functions
	KXK,   // Generic "mate lone king" eval
	KBNK,  // KBN vs K
	KPK,   // KP vs K
	KRKP,  // KR vs KP
	KRKB,  // KR vs KB
	KRKN,  // KR vs KN
	KQKP,  // KQ vs KP
	KQKR,  // KQ vs KR
	KBBKN, // KBB vs KN
	KNNK,  // KNN vs K
	KmmKm, // K and two minors vs K and one or two minors

	// Scaling functions
	KBPsK,   // KB+pawns vs K
	KQKRPs,  // KQ vs KR+pawns
	KRPKR,   // KRP vs KR
	KRPPKRP, // KRPP vs KRP
	KPsK,    // King and pawns vs king
	KBPKB,   // KBP vs KB
	KBPPKB,  // KBPP vs KB
	KBPKN,   // KBP vs KN
	KNPK,    // KNP vs K
	KNPKB,   // KNP vs KB
	KPKP     // KP vs KP
};

// Base template for EndEvaluator (evalFunc and scalingFunc)
class EndEvaluatorBase
{
public:
	virtual ~EndEvaluatorBase() {}
	virtual Color strong_color() const = 0;  // get the stronger side
	virtual int operator()(const Position&) const = 0; // return either Value or ScaleFactor
};

template<EndgameType E>
class EndEvaluator : public EndEvaluatorBase
{
public:
	explicit EndEvaluator(Color c) : strongerSide(c), weakerSide(~c) {}
	Color strong_color() const { return strongerSide; }
	int operator()(const Position&) const {return 0;}; // return either Value or ScaleFactor
private:
	Color strongerSide, weakerSide;
};

namespace Endgame
{
	typedef std::map<U64, std::unique_ptr<EndEvaluatorBase>> Map;
	// key and evaluator function collection. Init at program startup.
	extern Map evalFuncMap; 
	extern Map scalingFuncMap; 

	void init();
	// add entries to the maps
	template<EndgameType E>
	void add_eval_func(const string& code);
	template<EndgameType E>
	void add_scaling_func(const string& code);

	// map[key].get() gets the original pointer from the unique_ptr<>
	inline EndEvaluatorBase* probe_eval_func(U64 key, EndEvaluatorBase** eeb)
	{  return *eeb= (evalFuncMap.count(key) ? evalFuncMap[key].get() : nullptr ); }

	inline EndEvaluatorBase* probe_scaling_func(U64 key, EndEvaluatorBase** eeb)
	{  return *eeb= (scalingFuncMap.count(key) ? scalingFuncMap[key].get() : nullptr ); }
}

#endif // __endgame_h__
