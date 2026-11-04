/* ------------------------------------------------------------------   */
/*      item            : InitiateCritical.hxx
        made by         : repa
        from template   : HardwareModuleTemplate.hxx (2026.05)
        date            : Mon Nov  2 11:38:12 2026
        category        : header file
        description     :
        changes         : Mon Nov  2 11:38:12 2026 first version
        language        : C++
        copyright       : (c)
*/

#pragma once
// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"

// include the dusime common classes, and some extra for hardware module
#include <dusime/dusime.h>
#include <dueca/CriticalActivity.hxx>
#include <dusime/HardwareModule.hxx>

// include headers for functions/classes you need in the module

/** A hardware module facility test.

    If a hardware module runs into an unrecoverable error, it can flag
    a "CriticalErrorNodeWide". DUECA should stop communicating externally,
    and bring down the hardware. This module tests these effects.

    The instructions to create an module of this class from the start
    script are:

    \verbinclude initiate-critical.scm
 */
class InitiateCritical : public dueca::HardwareModule
{
  /** self-define the module type, to ease writing the parameter table */
  typedef InitiateCritical _ThisModule_;

private: // simulation data
  // declare the data you need in your simulation

  /** A flag to remember that the safety activity approves of transition to
      working state. */
  bool hardware_ok;

  /** Run time before flagging the critical error. */
  double runtime;

  /** Breakup time */
  double breaktime;

public:
  /** Some logic */
  enum ExpectedStates {
    InitialSafe,
    WaitToGiveOK1,
    WaitToGiveOK2,
    WaitToGiveOK3,
    GivenOk,
    Inactive,
    HoldCurrent,
    Calibrate,
    Calibrated,
    Advance,
    ErrorTriggered,
    BackInSafe,
    Undefined
  };

private:
  /** Do a nodewide or local critical error */
  bool nodewide;

  /** Check on the logic */
  ExpectedStates estates;

private: // trim calculation data
  // declare the trim calculation data needed for your simulation
private: // snapshot data
  // declare, if you need, the room for placing snapshot data
private: // channel access
private:
  /** Clock */
  dueca::PeriodicAlarm myclock;

  /** Callback object for simulation calculation. */
  dueca::Callback<InitiateCritical> cb1, cb2, cb3;

  /** Activity for simulation calculation. */
  dueca::CriticalActivity do_calc;

  /** Activity for breakup initiation (simulate ctrl-c) */
  dueca::ActivityCallback do_breakup;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char *const classname;

  /** Return the initial condition table. */
  static const dueca::IncoTable *getMyIncoTable();

  /** Return the parameter table */
  static const dueca::ParameterTable *getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from the creation script. */
  InitiateCritical(dueca::Entity *e, const char *part,
                   const dueca::PrioritySpec &ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengthy initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~InitiateCritical();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const dueca::TimeSpec &ts);

  /** Request check on the timing. */
  bool checkTiming(const std::vector<int> &i);

public: // member functions for cooperation with DUECA
  /** indicate that, at least for initial start-up, everything is ready */
  bool isInitialPrepared();

  /** indicate that everything is ready. */
  bool isPrepared();

  /** initial start, go into safe mode. */
  void initialStartModule(const dueca::TimeSpec &ts);

  /** start responsiveness to input data. */
  void startModule(const dueca::TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const dueca::TimeSpec &time);

  /** also stop the safe mode. */
  void finalStopModule(const dueca::TimeSpec &time);

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const dueca::TimeSpec &ts);

  /** the method that implements the safety strategy. */
  void doSafeWork(const dueca::TimeSpec &ts);

  /** node breakup initiation */
  void doBreakup(const dueca::TimeSpec& ts);
};

std::ostream &operator<<(std::ostream &os,
                         const InitiateCritical::ExpectedStates &es);
