/* ------------------------------------------------------------------   */
/*      item            : WSWriteRead.hxx
        made by         : repa
        from template   : DusimeModuleTemplate.hxx (2022.06)
        date            : Wed Sep 11 04:26:52 2024
        category        : header file
        description     :
        changes         : Wed Sep 11 04:26:52 2024 first version
        language        : C++
        copyright       : (c)
*/

#ifndef WSWriteRead_hxx
#define WSWriteRead_hxx

// include the dusime header
#include "ChannelReadToken.hxx"
#include <dusime.h>
USING_DUECA_NS;

// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"

// include headers for functions/classes you need in the module
#include <array>

/** Fixed pair of write channel and two read channels.

    The instructions to create an module of this class from the start
    script are:

    \verbinclude ws-write-read.scm
*/
class WSWriteRead : public SimulationModule
{
  /** self-define the module type, to ease writing the parameter table */
  typedef WSWriteRead _ThisModule_;

private: // simulation data
  // current counter value
  int count;

  // maximum slack in reply delay
  int maxslack;

  // number of faults counted
  int nfault;

  /** Counters */
  std::array<int,6> nreceived;

private: // channel access
  // declare access tokens for all the channels you read and write

  /// Feedback from a process following all written data
  ChannelReadToken r_count1;

  /** Feedback from a python process repeatedly checking up on data. May have
      multiple repeated messages with same data
   */
  ChannelReadToken r_count2;

   /// Feedback from a process following all written data
  ChannelReadToken r_count1p;

  /** Feedback from a python process repeatedly checking up on data. May have
      multiple repeated messages with same data
   */
  ChannelReadToken r_count2p;

  /** Feedback from the preset write channel */
  ChannelReadToken r_preset1;

  /** Feedback from the preset write channel */
  ChannelReadToken r_preset2;

  /** Write data */
  ChannelWriteToken w_count;

private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  PeriodicAlarm myclock;

  /** Callback object for simulation calculation. */
  Callback<WSWriteRead> cb1;

  /** Activity for simulation calculation. */
  ActivityCallback do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char *const classname;

  /** Return the initial condition table. */
  static const IncoTable *getMyIncoTable();

  /** Return the parameter table. */
  static const ParameterTable *getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from the creation script. */
  WSWriteRead(Entity *e, const char *part, const PrioritySpec &ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengthy initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~WSWriteRead();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec &ts);

  /** Request check on the timing. */
  bool checkTiming(const std::vector<int> &i);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec &ts);
};

#endif
