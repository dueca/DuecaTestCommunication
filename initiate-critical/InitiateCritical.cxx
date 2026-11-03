/* ------------------------------------------------------------------   */
/*      item            : InitiateCritical.cxx
        made by         : repa
        from template   : HardwareModuleTemplate.cxx (2026.04)
        date            : Mon Nov  2 11:38:12 2026
        category        : body file
        description     :
        changes         : Mon Nov  2 11:38:12 2026 first version
        language        : C++
        copyright       : (c)
*/

// include the definition of the module class
#include "InitiateCritical.hxx"

// include additional files needed for your calculation here
#include <dueca/NodeManager.hxx>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dusime/dusime.h>
using namespace dueca;

// include the debug writing header. Warning and error messages
// are on by default, debug and info can be selected by
// uncommenting the respective defines
// #define D_MOD
// #define I_MOD
#include <dueca/debug.h>

// class/module name
const char *const InitiateCritical::classname = "initiate-critical";

// parameters to be inserted
const ParameterTable *InitiateCritical::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setTimeSpec),
      set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_, std::vector<int>>(
        &_ThisModule_::checkTiming),
      check_timing_description },

    { "runtime", new VarProbe<_ThisModule_, double>(&_ThisModule_::runtime),
      "Runtime [s], before flagging critical error" },

    { "breaktime", new VarProbe<_ThisModule_, double>(&_ThisModule_::breaktime),
      "Break-up time [s]" },

    { "nodewide", new VarProbe<_ThisModule_, bool>(&_ThisModule_::nodewide),
      "Flag the error as nodewide" },

    { NULL, NULL, "Test module for critical error flagging" }
  };

  return parameter_table;
}

// constructor
InitiateCritical::InitiateCritical(Entity *e, const char *part,
                                   const PrioritySpec &ps) :
  HardwareModule(e, classname, part, nullptr, 0),

  // initialize the data you need in your simulation
  hardware_ok(false),
  runtime(3600.0),
  breaktime(3600.0),
  nodewide(false),
  estates(InitialSafe),
  // initialize the data you need for the trim calculation

  // activity initialization
  myclock(),
  cb1(this, &_ThisModule_::doCalculation),
  cb2(this, &_ThisModule_::doSafeWork),
  cb3(this, &_ThisModule_::doBreakup),
  do_calc(this, "flag nodewide error", &cb1, &cb2, ps),
  do_breakup(getId(), "break-up simulation", &cb3, ps)
{
  // do the actions you need for the simulation

  // connect the triggers for simulation
  do_calc.setTrigger(myclock);
  do_breakup.setTrigger(myclock);
}

bool InitiateCritical::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
InitiateCritical::~InitiateCritical()
{
  //
}

// as an example, the setTimeSpec function
bool InitiateCritical::setTimeSpec(const TimeSpec &ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0)
    return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// and the checkTiming function
bool InitiateCritical::checkTiming(const std::vector<int> &i)
{
  if (i.size() == 3) {
    new TimingCheck(do_calc, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_calc, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

// tell DUECA you are prepared to start your safety work
bool InitiateCritical::isInitialPrepared()
{
  // do whatever checks on the hardware that you need before working
  // with it. Return true when things are (seem?) OK
  return true;
}

// tell DUECA you are prepared
bool InitiateCritical::isPrepared()
{
  // do whatever additional calculations you need to prepare the model.
  bool res = true;

  CHECK_CONDITION(hardware_ok);
  // return result of checks
  return res;
}

// start the module
void InitiateCritical::initialStartModule(const TimeSpec &time)
{
  hardware_ok = false;
  do_calc.switchOn(time);
}

// start the module
void InitiateCritical::startModule(const TimeSpec &time)
{
  do_calc.switchWork(time);
  do_breakup.switchOn(time);
}

// stop the module work
void InitiateCritical::stopModule(const TimeSpec &time)
{
  // unset the hardware_ok flag, so the safe strategy is forced to re-evaluate
  hardware_ok = false;
  do_calc.switchSafe(time);
}

// also stop the safe mode
void InitiateCritical::finalStopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
  do_breakup.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void InitiateCritical::doCalculation(const TimeSpec &ts)
{
  // check the state we are supposed to be in
  switch (getAndCheckState(ts)) {
  case SimulationState::Inactive: {
    if (estates == GivenOk || estates == Inactive) {
      estates = Inactive;
    }
    else {
      W_MOD("In inactive, unexpected state " << estates);
    }

  } break;

  case SimulationState::Inactive_HoldCurrent: {
    // do the transition from inactive to the position/state for the
    // hardware in HoldCurrent. Also repeat (possibly dummy) output, as
    // in HoldCurrent

    // if the state transition is complete, tell the module
    if (estates == Inactive) {
      transitionComplete();
    }
  } break;

  case SimulationState::HoldCurrent: {
    // only repeat the output, don not change the model state
    if (estates == Inactive || estates == Calibrated ||
        estates == HoldCurrent) {
      estates = HoldCurrent;
    }
    else {
      W_MOD("In HoldCurrent, unexpected state " << estates);
    }

  } break;

  case SimulationState::Calibrate_HoldCurrent: {
    // in this state, the calibration of the hardware has to be carried
    // Also repeat (possibly dummy) output, as in HoldCurrent
    if (estates == HoldCurrent) {
      estates = Calibrate;
    }
    else if (estates == Calibrate || estates == Calibrated) {
      estates = Calibrated;
      transitionComplete();
    }
    else {
      W_MOD("In HoldCurrent, unexpected state " << estates);
    }
  } break;

  case SimulationState::Replay:
    // In this template, the replay and advance modes are treated identical
    // If you record user input in this module, make a distinction there.
  case SimulationState::Advance: {
    if (estates == HoldCurrent) {
      estates = Advance;
    }
    else if (estates == Advance) {
      runtime -= ts.getDtInSeconds();
      if (runtime < 0.0) {
        do_calc.criticalErrorNodeWide();
        estates = ErrorTriggered;
      }
    }
    else {
      W_MOD("In Advance, unexpected state " << estates);
    }

    // do the simulation calculations, one stp
  } break;

  case SimulationState::Replay_HoldCurrent:
  case SimulationState::Advance_HoldCurrent: {

    // after reaching HoldCurrent position, call this
    transitionComplete();
  } break;

  case SimulationState::HoldCurrent_Inactive: {
    // reset the hardware to the inactive position, and keep outputting
    // those dummy values!

    // after reaching inactive position, call this
    transitionComplete();
  } break;

  default:
    // other states should never be entered, however, if in the future
    // the system is expanded, warn that things have gone wrong.
    // throwing the exception will cause the safe activity to be called
    // in the future
    throw CannotHandleState(getId(), GlobalId(), "state unhandled");
  }
}

void InitiateCritical::doSafeWork(const TimeSpec &ts)
{
  // you may not assume anything about communications, channels, etcetera
  // only control your hardware into a safe mode. No getAndCheckState either,
  // this runs completely alone, aside from comm with the hardware
  switch (estates) {
  case InitialSafe:
    estates = WaitToGiveOK1;
    break;
    break;
  case WaitToGiveOK1:
    estates = WaitToGiveOK2;
    break;
  case WaitToGiveOK2:
    estates = WaitToGiveOK3;
    break;
  case WaitToGiveOK3:
    estates = GivenOk;
    hardware_ok = true;
    break;
  case GivenOk:
    break;
  case ErrorTriggered:
    hardware_ok = false;
    W_MOD("Back in safe");
    estates = BackInSafe;
    break;
  case BackInSafe:
    break;
  default:
    E_MOD("InitiateCritical::doSafeWork unexpected state " << estates);
  }
}

void InitiateCritical::doBreakup(const TimeSpec &ts)
{
  breaktime -= ts.getDtInSeconds();
  if (breaktime <= 0.0) {
    NodeManager::single()->breakUp();
    breaktime = 10000000.0;
  }
}

std::ostream &operator<<(std::ostream &os,
                         const InitiateCritical::ExpectedStates &es)
{
  const char *names[] = { "InitialSafe",   "WaitToGiveOK1",  "WaitToGiveOK2",
                          "WaitToGiveOK3", "GivenOk",        "Inactive",
                          "HoldCurrent",   "Calibrate",      "Calibrated",
                          "Advance",       "ErrorTriggered", "BackInSafe",
                          "UnDefined" };
  return os << names[std::min(unsigned(es),
                              unsigned(InitiateCritical::Undefined))];
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<InitiateCritical> a(InitiateCritical::getMyParameterTable());
