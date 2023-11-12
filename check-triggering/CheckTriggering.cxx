/* ------------------------------------------------------------------   */
/*      item            : CheckTriggering.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx (2022.06)
        date            : Thu Nov  9 09:08:52 2023
        category        : body file
        description     :
        changes         : Thu Nov  9 09:08:52 2023 first version
        language        : C++
        copyright       : (c)
*/


#define CheckTriggering_cxx

// include the definition of the module class
#include "CheckTriggering.hxx"

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dueca.h>

// include the debug writing header, by default, write warning and
// error messages
#define W_MOD
#define E_MOD
#include <debug.h>

// class/module name
const char* const CheckTriggering::classname = "check-triggering";

// Parameters to be inserted
const ParameterTable* CheckTriggering::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_,TimeSpec>
        (&_ThisModule_::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_,std::vector<int> >
      (&_ThisModule_::checkTiming), check_timing_description },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "Sends loads of data and checks triggering performance"} };

  return parameter_table;
}

// constructor
CheckTriggering::CheckTriggering(Entity* e, const char* part, const
                   PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  ts_1_1(),
  nfault(0),

  w_s1_1(getId(),
         NameSet(getEntity(), getclassname<TriggerD>(), "stream_1_1"),
         getclassname<TriggerD>(), "stream_1_1", Channel::Continuous),
  w_e1_1(getId(),
         NameSet(getEntity(), getclassname<TriggerD>(), "event_1_1"),
         getclassname<TriggerD>(), "stream_1_1", Channel::Events),
  r_s1_1(getId(),
         NameSet(getEntity(), getclassname<TriggerD>(), "stream_1_1"),
         getclassname<TriggerD>(), 0, Channel::Continuous),
  r_e1_1(getId(),
         NameSet(getEntity(), getclassname<TriggerD>(), "event_1_1"),
         getclassname<TriggerD>(), 0, Channel::Events),
  r_s1_1_pp(getId(),
            NameSet(getEntity(), getclassname<TriggerD>(), "stream_1_1"),
            getclassname<TriggerD>(), 0, Channel::Continuous),
  r_e1_1_pp(getId(),
            NameSet(getEntity(), getclassname<TriggerD>(), "event_1_1"),
            getclassname<TriggerD>(), 0, Channel::Events),
  r_s1_1_pm(getId(),
            NameSet(getEntity(), getclassname<TriggerD>(), "stream_1_1"),
            getclassname<TriggerD>(), 0, Channel::Continuous),
  r_e1_1_pm(getId(),
            NameSet(getEntity(), getclassname<TriggerD>(), "event_1_1"),
            getclassname<TriggerD>(), 0, Channel::Events),

  cb0(this, &_ThisModule_::doSend_1_1),
  do_1_1(getId(), "stream+event trigger, 1step", &cb0, ps),

  cb1(this, &_ThisModule_::doCheck_s1_1),
  do_s1_1(getId(), "stream, 1/1", &cb1, ps),

  cb2(this, &_ThisModule_::doCheck_e1_1),
  do_e1_1(getId(), "event, 1/1", &cb2, ps),

  cb3(this, &_ThisModule_::doCheck_s1_1_pp),
  do_s1_1_pp(getId(), "stream, 1/1, high prio", &cb3,
             PrioritySpec(ps.getPriority()+1, 0)),

  cb4(this, &_ThisModule_::doCheck_e1_1_pp),
  do_e1_1_pp(getId(), "event, 1/1, high prio", &cb4,
             PrioritySpec(ps.getPriority()+1, 0)),

  cb5(this, &_ThisModule_::doCheck_s1_1_pm),
  do_s1_1_pm(getId(), "stream, 1/1, lower prio", &cb5,
             PrioritySpec(ps.getPriority()-1, 0)),

  cb6(this, &_ThisModule_::doCheck_e1_1_pm),
  do_e1_1_pm(getId(), "event, 1/1, lower prio", &cb6,
             PrioritySpec(ps.getPriority()-1, 0)),

  myclock()
{
  // connect the triggers for simulation
  do_1_1.setTrigger(myclock);
  do_s1_1.setTrigger(r_s1_1);
  do_e1_1.setTrigger(r_e1_1);
  do_s1_1_pp.setTrigger(r_s1_1_pp);
  do_e1_1_pp.setTrigger(r_e1_1_pp);
  do_s1_1_pm.setTrigger(r_s1_1_pm);
  do_e1_1_pm.setTrigger(r_e1_1_pm);
}

bool CheckTriggering::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
CheckTriggering::~CheckTriggering()
{
  //
}

// as an example, the setTimeSpec function
bool CheckTriggering::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  myclock.changePeriodAndOffset(ts);

  // return true if everything is acceptable
  return true;
}

// the checkTiming function installs a check on the activity/activities
// of the module
bool CheckTriggering::checkTiming(const std::vector<int>& i)
{
  if (i.size() == 3) {
    new TimingCheck(do_s1_1, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_s1_1, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

// tell DUECA you are prepared
bool CheckTriggering::isPrepared()
{
  bool res = true;

  CHECK_TOKEN(w_s1_1);
  CHECK_TOKEN(w_e1_1);

  CHECK_TOKEN(r_s1_1);
  CHECK_TOKEN(r_e1_1);

  CHECK_TOKEN(r_s1_1_pp);
  CHECK_TOKEN(r_e1_1_pp);

  CHECK_TOKEN(r_s1_1_pm);
  CHECK_TOKEN(r_e1_1_pm);

  // return result of checks
  return res;
}

// start the module
void CheckTriggering::startModule(const TimeSpec &time)
{
  do_1_1.switchOn(time);
  do_s1_1.switchOn(time);
  do_e1_1.switchOn(time);
  do_s1_1_pp.switchOn(time);
  do_e1_1_pp.switchOn(time);
  do_s1_1_pm.switchOn(time);
  do_e1_1_pm.switchOn(time);
}

// stop the module
void CheckTriggering::stopModule(const TimeSpec &time)
{
  do_1_1.switchOff(time);
  do_s1_1.switchOff(time);
  do_e1_1.switchOff(time);
  do_s1_1_pp.switchOff(time);
  do_e1_1_pp.switchOff(time);
  do_s1_1_pm.switchOff(time);
  do_e1_1_pm.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void CheckTriggering::doSend_1_1(const TimeSpec& ts)
{
  ts_1_1 = ts;
  DataWriter<TriggerD> ws(w_s1_1, ts);
  ws.data().ticktime = ts_1_1.getValidityStart();
  DataWriter<TriggerD> we(w_e1_1, ts);
  we.data().ticktime = ts_1_1.getValidityStart();
}

void CheckTriggering::doCheck_s1_1(const TimeSpec& ts)
{
  try {
    // is there data? Check it has just been sent
    DataReader<TriggerD> rs(r_s1_1, ts);
    if (rs.data().ticktime != ts_1_1.getValidityStart()) {
      W_MOD("doCheck_s1_1 mismatch, ts_1_1=" << ts_1_1 <<
            " ticktime=" << rs.data().ticktime);
      nfault++;
    }
  }
  catch (const std::exception& e) {
    W_MOD("Cannot read stream with timespec, at " << ts << " : " << e.what());
    nfault++;
  }
  try {
    // try read latest now
    DataReader<TriggerD,MatchIntervalStartOrEarlier> rs(r_s1_1);
    if (rs.timeSpec() != ts) {
      W_MOD("doCheck_s1_1 latest missmatch, data: " << rs.timeSpec() <<
            " act=" << ts);
      nfault++;
    }
  }
  catch (const std::exception& e) {
    W_MOD("doCheck_s1_1 cannot read stream latest, at " << ts << " : "
          << e.what());
    nfault++;
  }
}

void CheckTriggering::doCheck_e1_1(const TimeSpec& ts)
{
  DataTimeSpec tsrange(ts.getValidityStart(), 100);
  if (not r_e1_1.haveVisibleSets()) {
    W_MOD("doCheck_e1_1 expected haveVisibleSets()");
    nfault++;
  }
  if (not r_e1_1.getNumVisibleSets() == 1) {
    W_MOD("doCheck_e1_1 expected 1 getNumVisibleSets()");
    nfault++;
  }
  if (not r_e1_1.haveVisibleSets(ts)) {
    W_MOD("doCheck_e1_1 expected haveVisibleSets() with point ts");
    nfault++;
  }
  if (not r_e1_1.getNumVisibleSets(ts) == 1) {
    W_MOD("doCheck_e1_1 expected 1 getNumVisibleSets() with point ts");
    nfault++;
  }
  if (not r_e1_1.haveVisibleSets(tsrange)) {
    W_MOD("doCheck_e1_1 expected haveVisibleSets() with range ts");
    nfault++;
  }
  if (not r_e1_1.getNumVisibleSets(tsrange) == 1) {
    W_MOD("doCheck_e1_1 expected 1 getNumVisibleSets() with range ts");
    nfault++;
  }
  try {
    DataReader<TriggerD> re(r_e1_1, ts);
    if (re.data().ticktime != ts_1_1.getValidityStart()) {
      W_MOD("doCheck_e1_1 mismatch, ts_1_1=" << ts_1_1 <<
            " ticktime=" << re.data().ticktime);
      nfault++;
    }
    if (re.timeSpec().getValidityStart() != ts_1_1.getValidityStart()) {
       W_MOD("doCheck_e1_1 mismatch, ts_1_1=" << ts_1_1 <<
             " datatimespec=" << re.timeSpec());
      nfault++;
    }
    if (re.timeSpec().getValidityStart() != ts.getValidityStart()) {
       W_MOD("doCheck_e1_1 mismatch, ts=" << ts <<
             " datatimespec=" << re.timeSpec());
      nfault++;
    }
  }
  catch (const std::exception& e) {
    W_MOD("doCheck_e1_1 cannot read event seq, at " << ts << " : " << e.what());
    nfault++;
  }
  if (r_e1_1.haveVisibleSets(ts)) {
    W_MOD("doCheck_e1_1 should no longer haveVisibleSets()");
    nfault++;
  }
  if (not r_e1_1.getNumVisibleSets(ts) == 0) {
    W_MOD("doCheck_e1_1 expected 0 getNumVisibleSets()");
    nfault++;
  }
  if (r_e1_1.haveVisibleSets(ts)) {
    W_MOD("doCheck_e1_1 should no longer haveVisibleSets() with point ts");
    nfault++;
  }
  if (not r_e1_1.getNumVisibleSets(ts) == 0) {
    W_MOD("doCheck_e1_1 expected 0 getNumVisibleSets() with point ts");
    nfault++;
  }
  if (r_e1_1.haveVisibleSets(tsrange)) {
    W_MOD("doCheck_e1_1 should no longer haveVisibleSets() with range ts");
    nfault++;
  }
  if (not r_e1_1.getNumVisibleSets(tsrange) == 0) {
    W_MOD("doCheck_e1_1 expected 0 getNumVisibleSets() with range ts");
    nfault++;
  }
}

void CheckTriggering::doCheck_s1_1_pp(const TimeSpec& ts)
{
  try {
    // is there data? Check it has just been sent
    DataReader<TriggerD> rs(r_s1_1_pp, ts);
    if (rs.data().ticktime != ts_1_1.getValidityStart()) {
      W_MOD("doCheck_s1_1_pp mismatch, ts_1_1=" << ts_1_1 <<
            " ticktime=" << rs.data().ticktime);
      nfault++;
    }
  }
  catch (const std::exception& e) {
    W_MOD("doCheck_s1_1_pp cannot read stream with timespec, at " << ts <<
          " : " << e.what());
    nfault++;
  }
  try {
    // try read latest now
    DataReader<TriggerD,MatchIntervalStartOrEarlier> rs(r_s1_1_pp);
    if (rs.timeSpec() != ts) {
      W_MOD("doCheck_s1_1_pp latest missmatch, data: " << rs.timeSpec() <<
            " act=" << ts);
      nfault++;
    }
  }
  catch (const std::exception& e) {
    W_MOD("doCheck_s1_1_pp cannot read stream latest, at " << ts <<
          " : " << e.what());
    nfault++;
  }
}

void CheckTriggering::doCheck_e1_1_pp(const TimeSpec& ts)
{
  try {
    DataReader<TriggerD> re(r_e1_1_pp, ts);
    if (re.data().ticktime != ts_1_1.getValidityStart()) {
      W_MOD("doCheck_e1_1_pp mismatch, ts_1_1=" << ts_1_1 <<
            " ticktime=" << re.data().ticktime);
      nfault++;
    }
    if (re.timeSpec().getValidityStart() != ts_1_1.getValidityStart()) {
       W_MOD("doCheck_e1_1_pp mismatch, ts_1_1=" << ts_1_1 <<
             " datatimespec=" << re.timeSpec());
      nfault++;
    }
    if (re.timeSpec().getValidityStart() != ts.getValidityStart()) {
       W_MOD("doCheck_e1_1_pp mismatch, ts=" << ts <<
             " datatimespec=" << re.timeSpec());
      nfault++;
    }
  }
  catch (const std::exception& e) {
    W_MOD("doCheck_e1_1_pp cannot read event seq, at " << ts <<
          " : " << e.what());
    nfault++;
  }
}

void CheckTriggering::doCheck_s1_1_pm(const TimeSpec& ts)
{
  try {
    // is there data? Check it has just been sent
    DataReader<TriggerD> rs(r_s1_1_pm, ts);
    if (rs.data().ticktime != ts_1_1.getValidityStart()) {
      W_MOD("doCheck_s1_1_pm mismatch, ts_1_1=" << ts_1_1 <<
            " ticktime=" << rs.data().ticktime);
      nfault++;
    }
  }
  catch (const std::exception& e) {
    W_MOD("doCheck_s1_1_pm cannot read stream with timespec, at " << ts <<
          " : " << e.what());
    nfault++;
  }
  try {
    // try read latest now
    DataReader<TriggerD,MatchIntervalStartOrEarlier> rs(r_s1_1_pm);
    if (rs.timeSpec() != ts) {
      W_MOD("doCheck_s1_1_pm latest missmatch, data: " << rs.timeSpec() <<
            " act=" << ts);
      nfault++;
    }
  }
  catch (const std::exception& e) {
    W_MOD("doCheck_s1_1_pm cannot read stream latest, at " << ts <<
          " : " << e.what());
    nfault++;
  }
}

void CheckTriggering::doCheck_e1_1_pm(const TimeSpec& ts)
{
  try {
    DataReader<TriggerD> re(r_e1_1_pm, ts);
    if (re.data().ticktime != ts_1_1.getValidityStart()) {
      W_MOD("doCheck_e1_1_pm mismatch, ts_1_1=" << ts_1_1 <<
            " ticktime=" << re.data().ticktime);
      nfault++;
    }
    if (re.timeSpec().getValidityStart() != ts_1_1.getValidityStart()) {
       W_MOD("doCheck_e1_1_pm mismatch, ts_1_1=" << ts_1_1 <<
             " datatimespec=" << re.timeSpec());
      nfault++;
    }
    if (re.timeSpec().getValidityStart() != ts.getValidityStart()) {
       W_MOD("doCheck_e1_1_pm mismatch, ts=" << ts <<
             " datatimespec=" << re.timeSpec());
      nfault++;
    }
  }
  catch (const std::exception& e) {
    W_MOD("doCheck_e1_1_pm cannot read event seq, at " << ts <<
          " : " << e.what());
    nfault++;
  }
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
static TypeCreator<CheckTriggering> a(CheckTriggering::getMyParameterTable());
