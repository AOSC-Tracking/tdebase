/*
    Naughty applet - Runaway process monitor for the TDE panel

    Copyright 2000 Rik Hemsley (rikkus) <rik@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* OpenBSD support by Jean-Yves Burlett <jean-yves@burlett.org> */

#if defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/ucred.h>
#include <sys/sched.h>
#include <stdlib.h>
#endif

#ifdef __NetBSD__
#include <kvm.h>
#include <sys/sched.h>
#endif

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <tqfile.h>
#include <tqstring.h>
#include <tqstringlist.h>
#include <tqtextstream.h>
#include <tqdir.h>
#include <tqtimer.h>
#include <tqmap.h>
#include <tqdatetime.h>

#include <tdelocale.h>

#include "NaughtyProcessMonitor.h"

class NaughtyProcessMonitorPrivate
{
  public:

    NaughtyProcessMonitorPrivate()
      : interval_(0),
        timer_(0),
        oldLoad_(0),
        triggerLevel_(0)
    {
    }

    ~NaughtyProcessMonitorPrivate()
    {
      // Empty.
    }

    uint interval_;
    TQTimer * timer_;
    TQMap<ulong, uint> loadMap_;
    TQMap<ulong, uint> scoreMap_;
#if defined(__OpenBSD__) || defined(__NetBSD__)
    TQMap<ulong, uint> cacheLoadMap_;
    TQMap<ulong, uid_t> uidMap_;
#endif
#ifdef __NetBSD__
    kvm_t *kd;
#endif
    uint oldLoad_;
    uint triggerLevel_;

  private:

    NaughtyProcessMonitorPrivate(const NaughtyProcessMonitorPrivate &);

    NaughtyProcessMonitorPrivate & operator =
      (const NaughtyProcessMonitorPrivate &);
};

NaughtyProcessMonitor::NaughtyProcessMonitor
  (
   uint interval,
   uint triggerLevel,
   TQObject * parent,
   const char * name
  )
  : TQObject(parent, name)
{
  d = new NaughtyProcessMonitorPrivate;
  d->interval_ = interval * 1000;
  d->triggerLevel_ = triggerLevel;
  d->timer_ = new TQTimer(this, "NaughtyProcessMonitorPrivate::timer");
#ifdef __NetBSD__
  d->kd = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, "kvm_open");
#endif
  connect(d->timer_, TQT_SIGNAL(timeout()), this, TQT_SLOT(slotTimeout()));
}

NaughtyProcessMonitor::~NaughtyProcessMonitor()
{
#ifdef __NetBSD__
  kvm_close(d->kd);
#endif
  delete d;
}

  void
NaughtyProcessMonitor::start()
{
  d->timer_->start(d->interval_, true);
}

  void
NaughtyProcessMonitor::stop()
{
  d->timer_->stop();
}

  uint
NaughtyProcessMonitor::interval() const
{
  return d->interval_ / 1000;
}

  void
NaughtyProcessMonitor::setInterval(uint i)
{
  stop();
  d->interval_ = i * 1000;
  start();
}

  uint
NaughtyProcessMonitor::triggerLevel() const
{
  return d->triggerLevel_;
}

  void
NaughtyProcessMonitor::setTriggerLevel(uint i)
{
  d->triggerLevel_ = i;
}

  void
NaughtyProcessMonitor::slotTimeout()
{
  uint cpu = cpuLoad();

  emit(load(cpu));

  if (cpu > d->triggerLevel_ * (d->interval_ / 1000))
  {
    uint load;
    TQValueList<ulong> l(pidList());

    for (TQValueList<ulong>::ConstIterator it(l.begin()); it != l.end(); ++it)
      if (getLoad(*it, load))
        _process(*it, load);
  }

  d->timer_->start(d->interval_, true);
}

  void
NaughtyProcessMonitor::_process(ulong pid, uint load)
{
  if (!d->loadMap_.contains(pid))
  {
    d->loadMap_.insert(pid, load);
    return;
  }

  uint oldLoad = d->loadMap_[pid];
  bool misbehaving = (load - oldLoad) > 40 * (d->interval_ / 1000);
  bool wasMisbehaving = d->scoreMap_.contains(pid);

  if (misbehaving)
    if (wasMisbehaving)
    {
      d->scoreMap_.replace(pid, d->scoreMap_[pid] + 1);
      if (canKill(pid))
        emit(runawayProcess(pid, processName(pid)));
    }
    else
      d->scoreMap_.insert(pid, 1);
  else
    if (wasMisbehaving)
      d->scoreMap_.remove(pid);

  d->loadMap_.replace(pid, load);
}

// Here begins the set of system-specific methods.

  bool
NaughtyProcessMonitor::canKill(ulong pid) const
{
#ifdef __linux__
  TQFile f("/proc/" + TQString::number(pid) + "/status");

  if (!f.open(IO_ReadOnly))
    return false;

  TQTextStream t(&f);

  TQString s;

  while (!t.atEnd() && s.left(4) != "Uid:")
    s = t.readLine();

  TQStringList l(TQStringList::split('\t', s));

  uint a(l[1].toUInt());

// What are these 3 fields for ? Would be nice if the Linux kernel docs
// were complete, eh ?
//  uint b(l[2].toUInt()); 
//  uint c(l[3].toUInt());
//  uint d(l[4].toUInt());

  return geteuid() == a;
#elif defined(__OpenBSD__) || defined(__NetBSD__)
  // simply check if entry exists in the uid map and use it
  if (!d->uidMap_.contains(pid))
      return false ;
  
  return geteuid () == d->uidMap_[pid] ;
#else
  Q_UNUSED( pid );
  return false;
#endif
}

  TQString
NaughtyProcessMonitor::processName(ulong pid) const
{
#if defined(__linux__) || defined(__OpenBSD__) || defined(__NetBSD__)
#ifdef __linux__
  TQFile f("/proc/" + TQString::number(pid) + "/cmdline");

  if (!f.open(IO_ReadOnly))
    return i18n("Unknown");

  TQCString s;

  while (true)
  {
    int c = f.getch();

    // Stop at NUL
    if (c == -1 || char(c) == '\0')
      break;
    else
      s += char(c);
  }

 // Now strip 'tdeinit:' prefix.
  TQString unicode(TQString::fromLocal8Bit(s));

#elif defined(__NetBSD__)
  struct kinfo_proc2 *p;
  int len;
  char **argv;

  p = kvm_getproc2(d->kd, KERN_PROC_PID, pid,
                  sizeof(struct kinfo_proc2), &len);
  if (len < 1) {
      return i18n("Unknown");
  }

  // Now strip 'tdeinit:' prefix.
  TQString unicode(TQString::fromLocal8Bit(p->p_comm));

  if (unicode == "tdeinit") {
      argv = kvm_getargv2(d->kd, p, 100);
      while (argv != NULL && (*argv == "tdeinit:")) {
         argv++;
      }
      if (argv != NULL) {
         unicode = *argv;
      }
  }
#elif defined(__OpenBSD__)
  int mib[4] ;
  size_t size ;
  char **argv ;
  
  // fetch argv for the process `pid'

  mib[0] = CTL_KERN ;
  mib[1] = KERN_PROC_ARGS ;
  mib[2] = pid ;
  mib[3] = KERN_PROC_ARGV ;
  
  // we assume argv[0]'s size will be less than one page

  size = getpagesize () ;
  argv = (char **)calloc (size, sizeof (char)) ;
  size-- ; // ensure argv is ended by 0
  if (-1 == sysctl (mib, 4, argv, &size, NULL, 0)) {
      free (argv) ;
      return i18n("Unknown") ;
  }
  
 // Now strip 'tdeinit:' prefix.
  TQString unicode(TQString::fromLocal8Bit(argv[0]));

  free (argv) ;
#endif

  TQStringList parts(TQStringList::split(' ', unicode));

  TQString processName = parts[0] == "tdeinit:" ? parts[1] : parts[0];

  int lastSlash = processName.findRev('/');

  // Get basename, if there's a path.
  if (-1 != lastSlash)
    processName = processName.mid(lastSlash + 1);

  return processName;

#else
  Q_UNUSED( pid );
  return TQString::null;
#endif
}

  uint
NaughtyProcessMonitor::cpuLoad() const
{
#ifdef __linux__
  TQFile f("/proc/stat");

  if (!f.open(IO_ReadOnly))
    return 0;

  bool forgetThisOne = 0 == d->oldLoad_;

  TQTextStream t(&f);

  TQString s = t.readLine();

  TQStringList l(TQStringList::split(' ', s));

  uint user  = l[1].toUInt();
  uint sys   = l[3].toUInt();

  uint load = user + sys;
  uint diff = load - d->oldLoad_;
  d->oldLoad_ = load;

  return (forgetThisOne ? 0 : diff);
#elif defined(__OpenBSD__) || defined(__NetBSD__)
  int mib[2] ;
#ifdef __NetBSD__
  u_int64_t cp_time[CPUSTATES] ;
#else
  long cp_time[CPUSTATES] ;
#endif
  size_t size ;
  uint load, diff ;
  bool forgetThisOne = 0 == d->oldLoad_;

  // fetch CPU time statistics

  mib[0] = CTL_KERN ;
  mib[1] = KERN_CP_TIME ;

  size = CPUSTATES * sizeof(cp_time[0]) ;
  
  if (-1 == sysctl (mib, 2, cp_time, &size, NULL, 0))
      return 0 ;
  
  load = cp_time[CP_USER] + cp_time[CP_SYS] ;
  diff = load - d->oldLoad_ ;
  d->oldLoad_ = load ;
  
  return (forgetThisOne ? 0 : diff);
#else
  return 0;
#endif
}

  TQValueList<ulong>
NaughtyProcessMonitor::pidList() const
{
#ifdef __linux__
  TQStringList dl(TQDir("/proc").entryList());

  TQValueList<ulong> pl;

  for (TQStringList::ConstIterator it(dl.begin()); it != dl.end(); ++it)
    if (((*it)[0].isDigit()))
      pl << (*it).toUInt();

  return pl;
#elif defined(__NetBSD__)
  struct kinfo_proc2 *kp;
  int nentries;
  int i;
  TQValueList<ulong> l;

  kp = kvm_getproc2(d->kd, KERN_PROC_ALL, 0,
                  sizeof(struct kinfo_proc2), &nentries);

  // time statictics and euid data are fetched only for proceses in
  // the pidList, so, instead of doing on sysctl per process for
  // getLoad and canKill calls, simply cache the data we already have.

  d->cacheLoadMap_.clear();
  d->uidMap_.clear();
  for (i = 0; i < nentries; i++) {
      i << (unsigned long) kp[i].p_pid;
      d->cacheLoadMap_.insert (kp[i].p_pid,
                              kp[i].p_cpticks);
      d->uidMap_.insert (kp[i].p_pid,
                        kp[i].p_uid);
  }

  return l;

#elif defined(__OpenBSD__)
  int mib[3] ;
  int nprocs = 0, nentries ;
  size_t size ;
  struct kinfo_proc *kp ;
  int i ;
  TQValueList<ulong> l;

  // fetch number of processes

  mib[0] = CTL_KERN ;
  mib[1] = KERN_NPROCS ;
  
  if (-1 == sysctl (mib, 2, &nprocs, &size, NULL, 0))
      return l ;
  
  // magic size evaluation ripped from ps

  size = (5 * nprocs * sizeof(struct kinfo_proc)) / 4 ;
  kp = (struct kinfo_proc *)calloc (size, sizeof (char)) ;
  
  // fetch process info

  mib[0] = CTL_KERN ;
  mib[1] = KERN_PROC ;
  mib[2] = KERN_PROC_ALL ;
  
  if (-1 == sysctl (mib, 3, kp, &size, NULL, 0)) {
      free (kp) ;
      return l ;
  }
  
  nentries = size / sizeof (struct kinfo_proc) ;
  
  // time statistics and euid data are fetched only for processes in
  // the pidList, so, instead of doing one sysctl per process for
  // getLoad and canKill calls, simply cache the data we already have.

  d->cacheLoadMap_.clear () ;
  d->uidMap_.clear () ;
  for (i = 0; i < nentries; i++) {
#ifdef __OpenBSD__
      l << (unsigned long) kp[i].p_pid ;
      d->cacheLoadMap_.insert (kp[i].p_pid,
                              (kp[i].p_uticks +
                               kp[i].p_sticks)) ;
      d->uidMap_.insert (kp[i].p_pid,
                        kp[i].p_uid) ;
#else
      l << (unsigned long) kp[i].kp_proc.p_pid ;
      d->cacheLoadMap_.insert (kp[i].kp_proc.p_pid,
			       (kp[i].kp_proc.p_uticks + 
				kp[i].kp_proc.p_sticks)) ;
      d->uidMap_.insert (kp[i].kp_proc.p_pid,
			 kp[i].kp_eproc.e_ucred.cr_uid) ;
#endif
  }

  free (kp) ;
  
  return l ;
#else
  TQValueList<ulong> l;
  return l;
#endif
}

  bool
NaughtyProcessMonitor::getLoad(ulong pid, uint & load) const
{
#ifdef __linux__
  TQFile f("/proc/" + TQString::number(pid) + "/stat");

  if (!f.open(IO_ReadOnly))
    return false;

  TQTextStream t(&f);

  TQString line(t.readLine());

  TQStringList fields(TQStringList::split(' ', line));

  uint userTime (fields[13].toUInt());
  uint sysTime  (fields[14].toUInt());

  load = userTime + sysTime;

  return true;
#elif defined(__OpenBSD__) || defined(__NetBSD__)
  // use cache
  if (!d->cacheLoadMap_.contains(pid))
      return false ;
  
  load = d->cacheLoadMap_[pid] ;
  return true ;
#else
  Q_UNUSED( pid );
  Q_UNUSED( load );
  return false;
#endif
}

  bool
NaughtyProcessMonitor::kill(ulong pid) const
{
#if defined(__linux__) || defined(__OpenBSD__) || defined(__NetBSD__)
  return 0 == ::kill(pid, SIGKILL);
#else
  Q_UNUSED( pid );
  return false;
#endif
}

#include "NaughtyProcessMonitor.moc"
