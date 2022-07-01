/* ------------------------------------------------------------------   */
/*      item            : WriteUnified.hxx
        made by         : repa
        from template   : DusimeModuleTemplate.hxx
        template made by: Rene van Paassen
        date            : Mon Nov 15 17:20:23 2004
        category        : header file
        description     :
        changes         : Mon Nov 15 17:20:23 2004 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
*/

#ifndef WriteUnified_hxx
#define WriteUnified_hxx

// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"

// include the dusime header
#include <dusime.h>
#include <CriticalActivity.hxx>


// include headers for functions/classes you need in the module
#include <list>

/** A module to test the UnifiedChannel facilities of DUECA. It
    produces some initially-present blips, driven by a random-walk
    process, and it can produce bips that are inserted and removed,
    likewise driven by the random walk.

    The instructions to create an module of this class from the Scheme
    script are:

    \verbinclude write-multi-stream.scm
*/
class WriteUnified: public SimulationModule
{
private: // simulation data
  // declare the data you need in your simulation
  double dt;

  struct NormalBlipSpec
  {
    /** initial location/name blip. */
    MyBlip b;

    /** event type or not */
    bool evtype;

    /** Pointer to an access token. */
    ChannelWriteToken* token;

    /** Data recorder for advance (random input)/Replay */
    //DataRecorder drive_recorder;

    // constructor
    NormalBlipSpec(const MyBlip& b, ChannelWriteToken* token,
                   bool evtype = false) : b(b), evtype(evtype), token(token){}

    // access to the token.
    inline ChannelWriteToken*& getToken() {return token;}

    // access to the initial blip
    inline MyBlip& getBlip() {return b;}
  private:
    /** Prevent copy constructor */
    NormalBlipSpec(const NormalBlipSpec&);
  };


  class FlasherBlipSpec
  {
    /** initial location/name blip. */
    MyBlip b;

    /** Period in time ticks for the appearance and disappearance of
        this blip. */
    unsigned int period;

    /** Countdown counter, */
    unsigned int countdown;

    /** Pointer to an access token. */
    ChannelWriteToken* token;

  public:
    // constructor
    FlasherBlipSpec(const MyBlip& b);

    bool flash();

    // access to the token.
    ChannelWriteToken*& getToken();

    // access to the initial blip
    MyBlip& getBlip();

    // change period
    void setPeriod(unsigned int period);
  };

private: // trim calculation data
  // declare the trim calculation data needed for your simulation

private: // snapshot data
  dueca::smartstring            snapdata;

private: // channel access
  typedef list<NormalBlipSpec>  BlipList;
  BlipList bliplist;

  typedef list<FlasherBlipSpec> FlasherList;
  FlasherList flasherlist;

private: // activity allocation
  /** Callback object for simulation calculation. */
  Callback<WriteUnified>  cb1, cb2;

  /** Activity */
  ActivityCallback      do_calc;

  /** Regular clock activation. */
  PeriodicAlarm         myclock;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char* const           classname;

  /** Return the initial condition table. */
  static const IncoTable*            getMyIncoTable();

  /** Return the parameter table. */
  static const ParameterTable*       getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from scheme/the creation script. */
  WriteUnified(Entity* e, const char* part, const PrioritySpec& ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengty initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~WriteUnified();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec& ts);

  /** Request check on the timing. */
  bool checkTiming(const vector<int>& i);

  /** Add an object to the stream. */
  bool addBlip(const vstring& s);

  /** Add an object and indicate it produces an event type data */
  bool addEventBlip(const vstring& s);

  /** Put the last object added in its place. */
  bool placeBlip(const vector<float>& xyuv);

  /** Add an object to the stream. */
  bool addFlasherBlip(const vstring& s);

  /** Put the last object added in its place. */
  bool placeFlasherBlip(const vector<float>& xyuv);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** indicate that everything is ready. */
  bool isInitialPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

  /** start responsiveness to input data. */
  void initialStartModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void finalStopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** Method that does safe work */
  void doSafe(const TimeSpec& ts);

  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec& ts);

public: // member functions for cooperation with DUSIME
  /** For the Snapshot capability, fill the snapshot "snap" with the
      data saved at a point in your simulation (if from_trim is false)
      or with the state data calculated in the trim calculation (if
      from_trim is true). */
  void fillSnapshot(const TimeSpec& ts,
                    Snapshot& snap, bool from_trim);

  /** Restoring the state of the simulation from a snapshot. */
  void loadSnapshot(const TimeSpec& t, const Snapshot& snap);

  /** Perform a trim calculation. Should NOT use current state
      uses event channels parallel to the stream data channels,
      calculates, based on the event channel input, the steady state
      output. */
  void trimCalculation(const TimeSpec& ts, const TrimMode& mode);
};

#endif
