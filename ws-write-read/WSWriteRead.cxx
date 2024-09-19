/* ------------------------------------------------------------------   */
/*      item            : WSWriteRead.cxx
        made by         : repa
        from template   : DusimeModuleTemplate.cxx (2022.06)
        date            : Wed Sep 11 04:26:52 2024
        category        : body file
        description     :
        changes         : Wed Sep 11 04:26:52 2024 first version
        language        : C++
        copyright       : (c)
*/

#include "CommObjectTraits.hxx"
#include "DataReader.hxx"
#include "DataWriter.hxx"
#include "SimpleCounter.hxx"
#define WSWriteRead_cxx
// include the definition of the module class
#include "WSWriteRead.hxx"

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dusime.h>

// include the debug writing header. Warning and error messages
// are on by default, debug and info can be selected by
// uncommenting the respective defines
// #define D_MOD
// #define I_MOD
#include <debug.h>

// class/module name
const char *const WSWriteRead::classname = "ws-write-read";

// parameters to be inserted
const ParameterTable *WSWriteRead::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {

    { "set-timing",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setTimeSpec),
      set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_, std::vector<int>>(
        &_ThisModule_::checkTiming),
      check_timing_description },

    { "ncycles", new VarProbe<_ThisModule_, int>(&_ThisModule_::count),
      "Number of messages to send" },

    { "maximum-slack", new VarProbe<_ThisModule_, int>(&_ThisModule_::maxslack),
      "Number of cycles that the reply may be lagging" },

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL,
      "Test module for websocket communication.\n"
      "  Creates a channel to drive socket output, and checks on two read\n"
      "  channels whether the resulting reply is correct." }
  };

  return parameter_table;
}

// constructor
WSWriteRead::WSWriteRead(Entity *e, const char *part, const PrioritySpec &ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments.
     If you give a NULL pointer instead of the inco table, you will not be
     called for trim condition calculations, which is normal if you for
     example implement logging or a display.
     If you give 0 for the snapshot state, you will not be called to
     fill a snapshot, or to restore your state from a snapshot. Only
     applicable if you have no state. */
  SimulationModule(e, classname, part, NULL, 0),

  // initialize the data you need in your simulation
  count(10),
  maxslack(1),
  nfault(0),
  nreceived({ { 0, 0, 0, 0, 0, 0 } }),

  // initialize the channel access tokens, check the documentation for the
  // various parameters. Some examples:
  r_count1(getId(),
           NameSet(getEntity(), getclassname<SimpleCounter>(), "from_read"),
           getclassname<SimpleCounter>(), 0, Channel::Events),
  r_count2(getId(),
           NameSet(getEntity(), getclassname<SimpleCounter>(), "from_current"),
           getclassname<SimpleCounter>(), 0, Channel::Events),
  r_count1p(getId(),
            NameSet(getEntity(), getclassname<SimpleCounter>(), "from_read"),
            getclassname<SimpleCounter>(), 1, Channel::Events),
  r_count2p(getId(),
            NameSet(getEntity(), getclassname<SimpleCounter>(), "from_current"),
            getclassname<SimpleCounter>(), 1, Channel::Events),
  r_preset1(getId(),
            NameSet(getEntity(), getclassname<SimpleCounter>(), "preset1"),
            getclassname<SimpleCounter>(), 0, Channel::Events),
  r_preset2(getId(),
            NameSet(getEntity(), getclassname<SimpleCounter>(), "preset2"),
            getclassname<SimpleCounter>(), 0, Channel::Events),
  w_count(getId(), NameSet(getEntity(), getclassname<SimpleCounter>(), ""),
          getclassname<SimpleCounter>(), "", Channel::Events),

  // activity initialization
  myclock(),
  cb1(this, &_ThisModule_::doCalculation),
  do_calc(getId(), "write and read for websock", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(myclock);
}

bool WSWriteRead::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
WSWriteRead::~WSWriteRead()
{
  //
}

// as an example, the setTimeSpec function
bool WSWriteRead::setTimeSpec(const TimeSpec &ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0)
    return false;

  // specify the timespec to the activity
  // do_calc.setTimeSpec(ts);
  // or do this with the clock if you have it (don't do both!)
  myclock.changePeriodAndOffset(ts);

  // return true if everything is acceptable
  return true;
}

// and the checkTiming function
bool WSWriteRead::checkTiming(const std::vector<int> &i)
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

// tell DUECA you are prepared
bool WSWriteRead::isPrepared()
{
  bool res = true;

#if 0
  // the read tokens are only valid after the extenal client connected
  CHECK_TOKEN(r_count1);
  CHECK_TOKEN(r_count2);
  CHECK_TOKEN(r_count1p);
  CHECK_TOKEN(r_count2p);
#endif
  CHECK_TOKEN(w_count);
  CHECK_TOKEN(r_preset1);
  CHECK_TOKEN(r_preset2);

  // return result of checks
  return res;
}

// start the module
void WSWriteRead::startModule(const TimeSpec &time) { do_calc.switchOn(time); }

// stop the module
void WSWriteRead::stopModule(const TimeSpec &time) { do_calc.switchOff(time); }

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void WSWriteRead::doCalculation(const TimeSpec &ts)
{
  // first check up on data
  if (r_count1.isValid() && r_count1.haveVisibleSets(ts)) {
    DataReader<SimpleCounter> sc(r_count1, ts);
    nreceived[0]++;
    if (sc.data().count > count + maxslack) {
      W_MOD("WSWriteRead check 1, got " << sc.data().count << " while at "
                                        << count);
      nfault++;
    }
  }

  if (r_count2.isValid() && r_count2.haveVisibleSets(ts)) {
    DataReader<SimpleCounter> sc(r_count2, ts);
    nreceived[1]++;
    if (sc.data().count > count + maxslack) {
      W_MOD("WSWriteRead check 2, got " << sc.data().count << " while at "
                                        << count);
      nfault++;
    }
  }
  // first check up on data
  if (r_count1p.isValid() && r_count1p.haveVisibleSets(ts)) {
    DataReader<SimpleCounter> sc(r_count1p, ts);
    nreceived[2]++;
    if (sc.data().count > count + maxslack) {
      W_MOD("WSWriteRead check 1p, got " << sc.data().count << " while at "
                                         << count);
      nfault++;
    }
  }

  if (r_count2p.isValid() && r_count2p.haveVisibleSets(ts)) {
    DataReader<SimpleCounter> sc(r_count2p, ts);
    nreceived[3]++;
    if (sc.data().count > count + maxslack) {
      W_MOD("WSWriteRead check 2p, got " << sc.data().count << " while at "
                                         << count);
      nfault++;
    }
  }

  if (r_preset1.haveVisibleSets(ts)) {
    DataReader<SimpleCounter> sc(r_preset1, ts);
    nreceived[4]++;
    if (sc.data().count > count + maxslack) {
      W_MOD("WSWriteRead check preset1, got " << sc.data().count << " while at "
                                              << count);
      nfault++;
    }
  }

  if (r_preset2.haveVisibleSets(ts)) {
    DataReader<SimpleCounter> sc(r_preset2, ts);
    nreceived[5]++;
    if (sc.data().count > count + maxslack) {
      W_MOD("WSWriteRead check preset1, got " << sc.data().count << " while at "
                                              << count);
      nfault++;
    }
  }



  // check the state we are supposed to be in
  switch (getAndCheckState(ts)) {
  case SimulationState::HoldCurrent: {

    break;
  }

  case SimulationState::Replay:
  case SimulationState::Advance: {

    if (count) {
      count--;
      DataWriter<SimpleCounter> sc(w_count, ts);
      sc.data().count = count;
    }
    break;
  }
  default:
    // other states should never be entered for a SimulationModule,
    // HardwareModules on the other hand have more states. Throw an
    // exception if we get here,
    throw CannotHandleState(getId(), GlobalId(), "state unhandled");
  }
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<WSWriteRead> a(WSWriteRead::getMyParameterTable());
