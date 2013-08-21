Jarvis Initiative :: Project Excalibur
A pretty good chess engine.

(c) 2013  Jim Fan

The silent war storms the enchanted board
The lonely warrior wields the sacred sword
Behold! A symphony of black and white
Echoes a millennium of masters' might
Let wisdom inspire us in the glorious test
May Excalibur bless us on the lofty quest


Hope you enjoy my thematic poem.


======================== Introduction =========================

Excalibur is a chess engine that plays at grandmaster level. It is developed by Jim Fan, a college rising sophomore at the time of this writing. 
The engine is implemented entirely in C++. It contains around 10,000 lines of source code, taking into account the platform differences. 

The position evaluator and search optimizations are loosely adapted from the top engine Stockfish. Because of the superior parameters and formulas contributed by the open source community, Excalibur is occasionally able to beat some of the strongest engines on the planet, including but not limited to Houdini 3 (CCRL rank #1), Critter 1.6, Deep Rybka 4 and Shredder 12, which in turn can easily beat the human champions. 
Also my applause to the computer chess wiki (http://chessprogramming.wikispaces.com/) that armed me with all the theoretical knowledge. 

The ELO of Excalibur hasn't yet been decided, but a conservative estimate should probably be 2600. 

Current version: 1.0 beta. 


======================== Terms of Use =======================

Excalibur is intended to be a student project for educational purpose only, which means that any user can freely distribute and share it. The restriction is that any distribution should retain this README and file integrity. 

Excalibur is released free of charge and should NOT be used for any commercial purposes without the author's explicit written permission. 


========================= Installation =========================

Excalibur is a portable console application available on 3 most common desktop operating systems: Windows, Mac OS X and Linux.

It supports the UCI (Universal Chess Interface) protocol, which means that it can be used with any chessboard GUI (Graphical User Interface) compatible with UCI protocol. 
If you're comfortable enough with command line interaction, you can refer to the documentation section below to play a game in the text console manually.

For visually-inclined users: you can of course use commercial GUIs like Fritz or Shredder, but I highly recommend GNU Xboard, an excellent open source chessboards (totally free). The board graphics is a pleasure and the software functionality can satisfy the needs of either amateur or serious chessplayers.

http://www.gnu.org/software/xboard/
For your convenience, I have included the installers of Xboard for different operating systems you might have. 

After installation, start Xboard and go to menu => Engine => Load first/second engine => choose the Excalibur executable from your file system. Note that the "first engine" plays white and the "second" plays black. Or you can play against the engine yourself.

Handy keyboard shortcuts:

Ctrl + T == engine VS engine
Ctrl + W == engine VS human, engine white
Ctrl + B == engine VS human, engine black 
Ctrl + M == force engine to stop thinking and move immediately


========================= Distribution =========================

All three OS releases include a pdf copy of the UCI protocol reference, a perft test suite ("perftsuite.epd") and an Excalibur-style opening book ("Excalibur.book"), which I will explain later on. 

Platform-specific files:

[[[ Windows : tested on Windows 7 ]]]

-> Excalibur.exe: compiled by MSVC++ 12

-> Winboard-4.7-portable.zip: portable version of Winboard. The zip also includes other free/ open source engines like Crafty, Critter, Fruit, GNUchess, and Gull. You can play Excalibur against them and entertain with the breath-taking engine tournaments. 


[[[ Mac X: tested on 10.7 Mountain Lion ]]]

-> Excalibur-mac: compiled by clang++ 3.3

-> XQuartz-2.7.4.dmg: contains the X11 required by Mac xboard. If you haven't already installed x11 or don't know what x11 is, simply run the installer. 

-> Xboard-4.6.2-mac.zip: after XQuartz, unzip and double click the ".pkg"  installer. Includes Fruit 2.1


[[[ Linux: tested on Ubuntu 12 ]]]

-> Excalibur-linux: compiled by g++ 4.7

You can choose to install by "apt-get" from the internet (recommended) or by the included installer "Xboard-4.7.1-amd64.deb". Includes Fruit 2.1

Internet: sudo apt-get install xboard
	  sudo apt-get install polyglot (otherwise xboard won't have the UCI adaptor)

Installer: sudo dpkg -i Xboard-4.7.1-amd64.deb


========================= Settings =========================

Excalibur engine has a number of settings that would affect the game play. 
The settings can be changed in Xboard menu 'Engine' => '#1/#2 Engine Settings'

Setting manual: 

[Note] 
Material value is represented by "centipawn", which means that 100 units are one pawn worth of material.
Time is counted in milliseconds.

[1] Timing

Generally handled by the GUI. You can change the time control of a game by menu 'Options' => 'Time Control'

Excalibur supports the following UCI time control types (and their combination):

--> Sudden Death: finish all moves in a given amount of time
--> Moves To Go: finish x moves in a given amount of time before moving on to the next time control stage. 
--> Increment: gain a certain amount of time after making each move. 
--> Max Time per Move: fixed amount of time for each move.


Excalibur specific timing parameters:

--> "Time Usage": default 67, min 1, max 100. Percentage. Determines how much percent of the internally allocated time for this move should be used. If this amount of time passed, the engine would decide not to search one level deeper. 

Though the automated Excalibur time keeper does a very good job, you might want the engine to move faster than the "optimal" time. Under bullet games with "sudden death", the recommended time usage is around 30-50. Otherwise the default 0.67 works well. 
Lower the time usage at the risk of making hasty blunders. 

--> "Min Thinking Time": default 20, min 0, max 5000. the minimum amount of thinking time before making a move. 


[2] Evaluation
Default 100, min 0, max 200

The Evaluation parameters are percentage weights applied to various positional and tactical concerns.

--> Pawn Shield: how well are the pawns supporting each other
--> Mobility: knight, bishop, rook and queen mobility
--> Aggressiveness: maximize the threats to the opponent's king ring (the squares adjacent and semi-adjacent to the king)
--> King Safety: minimize the threats to our own king ring


[3] Contempt Factor
Default 0, min -50, max 50 (centipawn)

Determines the draw value (how much "contempt" we treat our opponent). Normally the draw is 0. Set to a positive value when we face a weaker opponent and wish to avoid draw as much as possible. Set to a negative value when we face a stronger opponent and try hard to at least draw the game.
Raise the contempt factor at the risk of losing. 


[4] Power Level
Default 10, min 0, max 10

Excalibur can play handicap. Level 10 means unlimited strength. Level 0 to 9 restrict the search depth/ time progressively. Lower power level means faster game play. 


[5] Opening Book
Excalibur has an opening book in its own format. 

--> "Use Opening Book": true/false
--> "Book Variation": true - we play a random opening line for the same position; 
false - we always play the best move in the book.
--> "Book File": default "Excalibur.book". Change the file path. 


==================== Console Commands ======================

The primary commands are case-insensitive. 

Besides the standard UCI protocol commands outlined in the pdf document, Excalibur supports a few more:

---> perft
stands for 'Perf'ormance 'T'est
perft takes a position and depth 'd' as input, and gives the number of all legal move combinations up to 'd' as output. 

usage: (1) use the UCI 'position' command to setup a position, then 'perft' without argument to enter depth interactively - any non-number input means quit. 
Or 'perft d' to return the result up to a depth. 

(2) 'perft filepath XXX': change the default "perftsuite.epd" to some other test suites.

'perft suite' : run a mass perft on the default file or the one specified by 'perft filepath'. This suite is used both for move generation verification and speed monitoring. Provided by the Rookie chess team (6500 tests). 

'perft suite XXX': run the mass perft in the file, starting at XXX instead of the beginning, which is 'initial'.

Use 'stop' to quit the mass perft. Note that you must wait until the current position is done.

(3) 'perft hash XXX': the perft speedometer can use a dedicated hash table to accelerate the test. Set hash size to XXX in megabytes. Max 4096 MB. 

'perft hash 0' to disable hash usage. 
'perft hash clear' to clear the hash table. 


---> 'd'/ 'disp'  and 'md'/ 'mdisp'
Display the internal board in ASCII graph. 'd' is the full pretty-print display and 'md' is the minimalist display.

---> 'magics'
Generate 64 magic keys for rook and bishop. The values are 64-bit hash keys used for "magic bitboard" technique, which calculates the rook/bishop attack map given a board occupancy. Excalibur's magic board allows it to generate moves fast.

---> Easter eggs
Now comes the tasty part. Easter eggs are definitely part of serious software engineering! ;)

Try the following commands yourself:

'sword'
'chess'
'Jim'
'clock'

There's one more and leave it to your own discovery. ;)


======================= Technicality ==========================

Notable techniques employed in Excalibur:

-> C++ object-oriented software design
-> Extensive use of template metaprogramming
-> Extensive use of STL data structures and algorithms
-> Magic bitboard representation
-> Pre-calculated bit masks and bitboard maps
-> LSB, MSB and bit-count assembly instruction
-> Zobrist keys (chess position hashing)
-> Standard compliant FEN string conversion
-> 16-bit move encoding
-> Specialized move generation for different types
-> Dedicated legal generator for fast perft and legality check
-> Perft hash
-> Multi-step static evaluation
-> Middle/ endgame score interpolation
-> Material and pawn hash table
-> Specialized endgame evaluators (20+)
-> User-defined evaluation weights
-> Multi-stage move ordering
-> Transposition table
-> Advanced alpha-beta tree searching
-> Null move pruning
-> Futility pruning and razoring
-> Late move reduction
-> Quiescence searching and static exchange evaluation
-> Iterative deepening
-> Multithreading support
-> Optimal dynamic time allocation, based on regression statistics
-> Full UCI protocol support and I/O handling
-> Opening book and polyglot adaptor
-> Handicap mode - power level
-> Platform-dependent implementation
-> Compilation opimizations


========================== History ===========================

In fall 2012, I decided to pursue a career in artificial intelligence, a branch of computer science that I deeply enjoy. Yet a college freshman, I thought of doing a personal project that can help polish my skills as well as keep my passion high. 

After a long time of hard trials to map out a plan, I accidentally came to revisit one of the ancient ambitions way back in my childhood. In those good old days, 64 squares of black and white were more than enough to trap my little mind in fascination.

Unfortunately, however, I was perhaps one of the least talented chess players in the world. Grandpa was my first and only trainer, but I never came close to beating him even when he played handicap (say, playing without a bishop).

That unfulfilled wish emerged from years of dust and finally secured my plan for the summer. In May 2013, I launched the project and set off on a journey to challenge myself with the "crown jewel" of artificial intelligence: chess.

I named my engine after the sacred sword of King Arthur, the legendary British king who led his knights to countless victories against foreign invaders.

And that's how Excalibur was born. 