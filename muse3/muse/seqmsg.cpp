//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: seqmsg.cpp,v 1.32.2.17 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <stdio.h>

#include "song.h"
#include "midiseq.h"
#include "midiport.h"
#include "minstrument.h"
#include "app.h"
#include "amixer.h"
#include "tempo.h"
///#include "sig.h"
#include "al/sig.h"
#include "audio.h"
#include "mididev.h"
#include "audiodev.h"
#include "alsamidi.h"
#include "audio.h"
#include "arranger.h"
#include "plugin.h"
#include "driver/jackmidi.h"
#include "midi_warn_init_pending_impl.h"
#include "gconfig.h"
#include "operations.h"
#include "ctrl.h"

namespace MusECore {

//---------------------------------------------------------
//   sendMsg
//---------------------------------------------------------

// this function blocks until the request has been processed
void Audio::sendMsg(AudioMsg* m)
      {
      static int sno = 0;

      if (_running) {
            m->serialNo = sno++;
            //DEBUG:
            msg = m;
            // wait for next audio "process" call to finish operation
            int no = -1;
            int rv = read(fromThreadFdr, &no, sizeof(int));
            if (rv != sizeof(int))
                  perror("Audio: read pipe failed");
            else if (no != (sno-1)) {
                  fprintf(stderr, "audio: bad serial number, read %d expected %d\n",
                     no, sno-1);
                  }
            }
      else {
            // if audio is not running (during initialization)
            // process commands immediately
            processMsg(m);
            }
      }

//---------------------------------------------------------
//   sendMessage
//    send request from gui to sequencer
//    wait until request is processed
//---------------------------------------------------------

bool Audio::sendMessage(AudioMsg* m, bool doUndo)
      {
      if (doUndo)
           MusEGlobal::song->startUndo();
      sendMsg(m);
      if (doUndo)
            MusEGlobal::song->endUndo(0);
      return false;
      }

//---------------------------------------------------------
//   msgRemoveRoute
//---------------------------------------------------------

void Audio::msgRemoveRoute(Route src, Route dst)
{
  //fprintf(stderr, "Audio::msgRemoveRoute:\n");
  msgRemoveRoute1(src, dst);
  MusEGlobal::song->connectJackRoutes(src, dst, true);
}

//---------------------------------------------------------
//   msgRemoveRoute1
//---------------------------------------------------------

void Audio::msgRemoveRoute1(Route src, Route dst)
      {
      AudioMsg msg;
      msg.id     = AUDIO_ROUTEREMOVE;
      msg.sroute = src;
      msg.droute = dst;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgAddRoute
//---------------------------------------------------------

void Audio::msgAddRoute(Route src, Route dst)
{
  //fprintf(stderr, "Audio::msgAddRoute:\n");
  msgAddRoute1(src, dst);
  MusEGlobal::song->connectJackRoutes(src, dst);
}

//---------------------------------------------------------
//   msgAddRoute1
//---------------------------------------------------------

void Audio::msgAddRoute1(Route src, Route dst)
      {
      AudioMsg msg;
      msg.id = AUDIO_ROUTEADD;
      msg.sroute = src;
      msg.droute = dst;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgAddPlugin
//---------------------------------------------------------

void Audio::msgAddPlugin(AudioTrack* node, int idx, PluginI* plugin)
      {
      AudioMsg msg;
      msg.id     = AUDIO_ADDPLUGIN;
      msg.snode  = node;
      msg.ival   = idx;
      msg.plugin = plugin;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetPrefader
//---------------------------------------------------------

void Audio::msgSetPrefader(AudioTrack* node, int val)
      {
      AudioMsg msg;
      msg.id    = AUDIO_SET_PREFADER;
      msg.snode = node;
      msg.ival  = val;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetChannels
//---------------------------------------------------------

void Audio::msgSetChannels(AudioTrack* node, int n)
      {
      if (n == node->channels())
            return;
      QString name = node->name();
      int mc       = std::max(n, node->channels());

      if (!name.isEmpty()) 
      {
            if (node->type() == Track::AUDIO_INPUT) 
            {
              if (!MusEGlobal::checkAudioDevice()) return;
              AudioInput* ai = (AudioInput*)node;
              for (int i = 0; i < mc; ++i) 
              {
                if (i < n && ai->jackPort(i) == 0) 
                {
                  char buffer[128];
                  snprintf(buffer, 128, "%s-%d", name.toLatin1().constData(), i);
                  ai->setJackPort(i, MusEGlobal::audioDevice->registerInPort(buffer, false));
                }
                else if ((i >= n) && ai->jackPort(i)) 
                {
                  RouteList* ir = node->inRoutes();
                  for (ciRoute ii = ir->begin(); ii != ir->end(); ++ii) 
                  {
                    Route r = *ii;
                    if ((r.type == Route::JACK_ROUTE) && (r.channel == i)) 
                    {
                      msgRemoveRoute(r, Route(node,i));
                      break;
                    }
                  }
                  MusEGlobal::audioDevice->unregisterPort(ai->jackPort(i));
                  ai->setJackPort(i, 0);
                }
              }      
            }
            else if (node->type() == Track::AUDIO_OUTPUT) 
            {
                  if (!MusEGlobal::checkAudioDevice()) return;
                  AudioOutput* ao = (AudioOutput*)node;
                  for (int i = 0; i < mc; ++i) 
                  {
                        void* jp = ao->jackPort(i);
                        if (i < n && jp == 0) 
                        {
                              char buffer[128];
                              snprintf(buffer, 128, "%s-%d", name.toLatin1().constData(), i);
                              ao->setJackPort(i, MusEGlobal::audioDevice->registerOutPort(buffer, false));
                        }
                        else if (i >= n && jp) 
                        {
                              RouteList* ir = node->outRoutes();
                              for (ciRoute ii = ir->begin(); ii != ir->end(); ++ii) 
                              {
                                    Route r = *ii;
                                    if ((r.type == Route::JACK_ROUTE) && (r.channel == i)) 
                                    {
                                          msgRemoveRoute(Route(node,i), r);
                                          break;
                                    }
                              }
                              MusEGlobal::audioDevice->unregisterPort(jp);
                              ao->setJackPort(i, 0);
                        }
                  }
            }
      }      
      
      // DELETETHIS 47
      /* TODO TODO: Change all stereo routes to mono. 
      // If we are going from stereo to mono we need to disconnect any stray synti 'mono last channel'...
      if(n == 1 && node->channels() > 1)
      {
        // This should always happen - syntis are fixed channels, user cannot change them. But to be safe...
        if(node->type() != Track::AUDIO_SOFTSYNTH) 
        {
          if(node->type() != Track::AUDIO_INPUT) 
          {
            RouteList* rl = node->inRoutes();
            for(iRoute r = rl->begin(); r != rl->end(); ++r)
            {
              // Only interested in synth tracks.
              if(r->type != Route::TRACK_ROUTE || r->track->type() != Track::AUDIO_SOFTSYNTH)
                continue;  
              // If it's the last channel...
              if(r->channel + 1 == ((AudioTrack*)r->track)->totalOutChannels())
              {
                msgRemoveRoute(*r, Route(node, r->channel));
                //msgRemoveRoute(r, Route(node, r->remoteChannel));
                break;
              }
            }
          }  
        
          if(node->type() != Track::AUDIO_OUTPUT) 
          {
            RouteList* rl = node->outRoutes();
            for(iRoute r = rl->begin(); r != rl->end(); ++r)
            {
              // Only interested in synth tracks.
              if(r->type != Route::TRACK_ROUTE || r->track->type() != Track::AUDIO_SOFTSYNTH)
                continue;  
              // If it's the last channel...
              if(r->channel + 1 == ((AudioTrack*)r->track)->totalOutChannels())
              {
                msgRemoveRoute(Route(node, r->channel), *r);
                //msgRemoveRoute(Route(node, r->remoteChannel), r);
                break;
              }
            }
          }  
        }   
      }
      */        
              
      AudioMsg msg;
      msg.id    = AUDIO_SET_CHANNELS;
      msg.snode = node;
      msg.ival  = n;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSwapControllerIDX
//---------------------------------------------------------

void Audio::msgSwapControllerIDX(AudioTrack* node, int idx1, int idx2)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_SWAP_CONTROLLER_IDX;
      msg.snode  = node;
      msg.a      = idx1;
      msg.b      = idx2;
      sendMsg(&msg);
}

//---------------------------------------------------------
//   msgClearControllerEvents
//---------------------------------------------------------

void Audio::msgClearControllerEvents(AudioTrack* node, int acid)
{
//       AudioMsg msg;
//       
//       msg.id     = AUDIO_CLEAR_CONTROLLER_EVENTS;
//       msg.snode  = node;
//       msg.ival   = acid;
//       sendMsg(&msg);
      
      
  ciCtrlList icl = node->controller()->find(acid);
  if(icl == node->controller()->end())
    return;
  
  CtrlList* cl = icl->second;
  if(cl->empty())
      return;
    
  CtrlList& clr = *icl->second;
      
  // The Undo system will take 'ownership' of these and delete them at the appropriate time.
  CtrlList* erased_list_items = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
  erased_list_items->insert(cl->begin(), cl->end());
    
  if(erased_list_items->empty())
  {
    delete erased_list_items;
    return;
  }

  MusEGlobal::song->applyOperation(UndoOp(UndoOp::ModifyAudioCtrlValList, node->controller(), erased_list_items, 0));
}

//---------------------------------------------------------
//   msgSeekPrevACEvent
//---------------------------------------------------------

void Audio::msgSeekPrevACEvent(AudioTrack* node, int acid)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_SEEK_PREV_AC_EVENT;
      msg.snode  = node;
      msg.ival   = acid;
      sendMsg(&msg);
}

//---------------------------------------------------------
//   msgSeekNextACEvent
//---------------------------------------------------------

void Audio::msgSeekNextACEvent(AudioTrack* node, int acid)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_SEEK_NEXT_AC_EVENT;
      msg.snode  = node;
      msg.ival   = acid;
      sendMsg(&msg);
}

//---------------------------------------------------------
//   msgEraseACEvent
//---------------------------------------------------------

void Audio::msgEraseACEvent(AudioTrack* node, int acid, int frame)
{
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteAudioCtrlVal, node, acid, frame));
}

//---------------------------------------------------------
//   msgEraseRangeACEvents
//---------------------------------------------------------

void Audio::msgEraseRangeACEvents(AudioTrack* node, int acid, int frame1, int frame2)
{
  ciCtrlList icl = node->controller()->find(acid);
  if(icl == node->controller()->end())
    return;
  
  CtrlList* cl = icl->second;
  if(cl->empty())
      return;
    
  if(frame2 < frame1)
  {
    const int tmp = frame1;
    frame1 = frame2;
    frame2 = tmp;
  }
      
  iCtrl s = cl->lower_bound(frame1);
  iCtrl e = cl->lower_bound(frame2);
  
  // No elements to erase?
  if(s == cl->end())
    return;
  
  CtrlList& clr = *icl->second;
      
  // The Undo system will take 'ownership' of these and delete them at the appropriate time.
  CtrlList* erased_list_items = new CtrlList(clr, CtrlList::ASSIGN_PROPERTIES);
  erased_list_items->insert(s, e);
    
  if(erased_list_items->empty())
  {
    delete erased_list_items;
    return;
  }

  MusEGlobal::song->applyOperation(UndoOp(UndoOp::ModifyAudioCtrlValList, node->controller(), erased_list_items, 0));
}

//---------------------------------------------------------
//   msgAddACEvent
//---------------------------------------------------------

void Audio::msgAddACEvent(AudioTrack* node, int acid, int frame, double val)
{
       MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddAudioCtrlVal, node, acid, frame, val));
}

//---------------------------------------------------------
//   msgChangeACEvent
//---------------------------------------------------------

void Audio::msgChangeACEvent(AudioTrack* node, int acid, int frame, int newFrame, double val)
{
      AudioMsg msg;
      
      msg.id     = AUDIO_CHANGE_AC_EVENT;
      msg.snode  = node;
      msg.ival   = acid;
      msg.a      = frame; 
      msg.b      = newFrame; 
      msg.dval   = val;
      sendMsg(&msg);
}

//---------------------------------------------------------
//   msgSeek
//---------------------------------------------------------

void Audio::msgSeek(const Pos& pos)
      {
      if (!MusEGlobal::checkAudioDevice()) return;
      MusEGlobal::audioDevice->seekTransport(pos);
      }

//---------------------------------------------------------
//   msgExecuteOperationGroup
//---------------------------------------------------------

void Audio::msgExecuteOperationGroup(Undo& operations)
{
	MusEGlobal::song->executeOperationGroup1(operations);
	
	AudioMsg msg;
	msg.id = SEQM_EXECUTE_OPERATION_GROUP;
	msg.operations=&operations;
	sendMsg(&msg);

	MusEGlobal::song->executeOperationGroup3(operations);
}

//---------------------------------------------------------
//   msgRevertOperationGroup
//---------------------------------------------------------

void Audio::msgRevertOperationGroup(Undo& operations)
{
	MusEGlobal::song->revertOperationGroup1(operations);
	
	
	AudioMsg msg;
	msg.id = SEQM_REVERT_OPERATION_GROUP;
	msg.operations=&operations;
	sendMsg(&msg);

	MusEGlobal::song->revertOperationGroup3(operations);
}

//---------------------------------------------------------
//   msgExecutePendingOperations
//   Bypass the Undo system and directly execute the pending operations.
//---------------------------------------------------------

void Audio::msgExecutePendingOperations(PendingOperationList& operations, bool doUpdate, SongChangedFlags_t extraFlags)
{
        if(operations.empty())
          return;
        AudioMsg msg;
        msg.id = SEQM_EXECUTE_PENDING_OPERATIONS;
        msg.pendingOps=&operations;
        sendMsg(&msg);
        operations.executeNonRTStage();
        const SongChangedFlags_t flags = operations.flags() | extraFlags;
        if(doUpdate && flags != 0)
        {
          MusEGlobal::song->update(flags);
          MusEGlobal::song->setDirty();
        }
}

//---------------------------------------------------------
//   msgPlay
//---------------------------------------------------------

void Audio::msgPlay(bool val)
      {
      if (val) {
            if (MusEGlobal::audioDevice)
            {
// REMOVE Tim. countin. Removed. This is not good. It's been here for years, and the idea 
//  was that the transport frame should start at what the user sees - the cursor tick position.
// But cursor tick position should follow audio frame position, not the other way around.
// The transport stops on some frame in-between ticks, and here virtually always it wants to jump back
//  to the cursor position's frame value upon play. It is almost never the same as the device frame position.
// Nothing should ever set the cursor position then seek the audio. All seeking is done
//  through audio, and gui is informed that way. This causes problems with audio sync callback: 
//  the seek happens while it is starting play mode, which it supports, but we don't want that.
// Since this only happens with OUR transport buttons and NOT any another Jack Transport client (QJackCtl),
//  it is a strong argument among others pro and con, to remove this for consistency with the other clients.
// Whatever issues removing this may cause should be fixable (they might be mistakes in the first place).
// User will need to be aware that the transport may be in-between ticks, ie. should view the frames,
//  which currently are really only seen via the BigTime window.
//                 unsigned sfr = MusEGlobal::song->cPos().frame();
//                 unsigned dcfr = MusEGlobal::audioDevice->getCurFrame();
//                 if(dcfr != sfr)
//                   MusEGlobal::audioDevice->seekTransport(MusEGlobal::song->cPos());
                MusEGlobal::audioDevice->startTransport();
            }
              
      }else {
            if (MusEGlobal::audioDevice)
                MusEGlobal::audioDevice->stopTransport();
            _bounce = false;
            }
      }


//---------------------------------------------------------
//   msgExternalPlay
//---------------------------------------------------------

void Audio::msgExternalPlay(bool val, bool doRewind)
      {
      if (val) {
            // Force the state to play immediately.
            state = PLAY;
            if (MusEGlobal::audioDevice)
            {
                //unsigned sfr = MusEGlobal::song->cPos().frame();
                //unsigned dcfr = MusEGlobal::audioDevice->getCurFrame();
                //if(dcfr != sfr)
                if(doRewind)
                  MusEGlobal::audioDevice->seekTransport(0);
                MusEGlobal::audioDevice->startTransport();
            }
              
      }else {
            state = STOP;
            if (MusEGlobal::audioDevice)
                MusEGlobal::audioDevice->stopTransport();
            _bounce = false;
            }
      }



//---------------------------------------------------------
//   msgRemoveTrack
//---------------------------------------------------------

void Audio::msgRemoveTrack(Track* track, bool doUndoFlag)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteTrack, MusEGlobal::song->tracks()->index(track), track), doUndoFlag);
      }

//---------------------------------------------------------
//   msgRemoveTracks
//    remove all selected tracks
//---------------------------------------------------------

void Audio::msgRemoveTracks()
{
      Undo operations;
      TrackList* tl = MusEGlobal::song->tracks();

      // NOTICE: This must be done in reverse order so that
      //          'undo' will repopulate in ascending index order!
      ciTrack it = tl->end();
      while(it != tl->begin())
      {
        --it;
        Track* tr = *it;
        if(tr->selected())
          operations.push_back(UndoOp(UndoOp::DeleteTrack, MusEGlobal::song->tracks()->index(tr), tr));
      }
      
      MusEGlobal::song->applyOperationGroup(operations);
}

//---------------------------------------------------------
//   msgMoveTrack
//    move track idx1 to slot idx2
//---------------------------------------------------------

void Audio::msgMoveTrack(int idx1, int idx2, bool doUndoFlag)
      {
      if (idx1 < 0 || idx2 < 0)   // sanity check
            return;
      int n = MusEGlobal::song->tracks()->size();
      if (idx1 >= n || idx2 >= n)   // sanity check
            return;
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::MoveTrack, idx1, idx2), doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddPart
//---------------------------------------------------------

void Audio::msgAddPart(Part* part, bool doUndoFlag)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddPart, part), doUndoFlag);
      }

//---------------------------------------------------------
//   msgRemovePart
//---------------------------------------------------------

void Audio::msgRemovePart(Part* part, bool doUndoFlag)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeletePart, part), doUndoFlag);
      }


//---------------------------------------------------------
//   msgAddEvent
//---------------------------------------------------------

void Audio::msgAddEvent(Event& event, Part* part, bool doUndoFlag, bool doCtrls, bool doClones)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddEvent, event,part, doCtrls, doClones), doUndoFlag);
      }

//---------------------------------------------------------
//   msgDeleteEvent
//---------------------------------------------------------

void Audio::msgDeleteEvent(Event& event, Part* part, bool doUndoFlag, bool doCtrls, bool doClones)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteEvent, event,part, doCtrls, doClones), doUndoFlag);
      }

//---------------------------------------------------------
//   msgChangeEvent
//---------------------------------------------------------

void Audio::msgChangeEvent(Event& oe, Event& ne, Part* part, bool doUndoFlag, bool doCtrls, bool doClones)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::ModifyEvent, ne,oe, part, doCtrls, doClones), doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddTempo
//---------------------------------------------------------

void Audio::msgAddTempo(int tick, int tempo, bool doUndoFlag)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddTempo, tick, tempo), doUndoFlag);
      }

//---------------------------------------------------------
//   msgSetGlobalTempo
//---------------------------------------------------------

void Audio::msgSetGlobalTempo(int val, bool doUndoFlag)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::SetGlobalTempo, val, 0), doUndoFlag);
      }

//---------------------------------------------------------
//   msgDeleteTempo
//---------------------------------------------------------

void Audio::msgDeleteTempo(int tick, int tempo, bool doUndoFlag)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteTempo, tick, tempo), doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddSig
//---------------------------------------------------------

void Audio::msgAddSig(int tick, int z, int n, bool doUndoFlag)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddSig, tick, z, n), doUndoFlag);
      }

//---------------------------------------------------------
//   msgRemoveSig
//! sends remove tempo signature message
//---------------------------------------------------------

void Audio::msgRemoveSig(int tick, int z, int n, bool doUndoFlag)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteSig, tick, z, n), doUndoFlag);
      }

//---------------------------------------------------------
//   msgAddKey
//---------------------------------------------------------

void Audio::msgAddKey(int tick, int key, bool doUndoFlag)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::AddKey, tick, key), doUndoFlag);
      }

//---------------------------------------------------------
//   msgRemoveKey
//! sends remove key message
//---------------------------------------------------------

void Audio::msgRemoveKey(int tick, int key, bool doUndoFlag)
      {
      MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteKey, tick, key), doUndoFlag);
      }


//---------------------------------------------------------
//   msgResetMidiDevices
//---------------------------------------------------------

void Audio::msgResetMidiDevices()
      {
      AudioMsg msg;
      msg.id = SEQM_RESET_DEVICES;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgInitMidiDevices
//---------------------------------------------------------

void Audio::msgInitMidiDevices(bool force)
      {
      //
      // test for explicit instrument initialization
      //
      
      if(!force && MusEGlobal::config.warnInitPending)
      {
        bool found = false;
        if(MusEGlobal::song->click())
        {
          MidiPort* mp = &MusEGlobal::midiPorts[MusEGlobal::clickPort];
          if(mp->device() && 
             (mp->device()->openFlags() & 1) && 
             mp->instrument() && !mp->instrument()->midiInit()->empty() &&
             !mp->initSent())
            found = true;
        }
        
        if(!found)
        {
          for(int i = 0; i < MIDI_PORTS; ++i)
          {
            MidiPort* mp = &MusEGlobal::midiPorts[i];
            if(mp->device() && (mp->device()->openFlags() & 1) && 
              mp->instrument() && !mp->instrument()->midiInit()->empty() &&
              !mp->initSent())
            {
              found = true;
              break;
            }
          }
        }
        
        if(found)
        {
          MusEGui::MidiWarnInitPendingDialog dlg;
          int rv = dlg.exec();
          bool warn = !dlg.dontAsk();
          if(warn != MusEGlobal::config.warnInitPending)  
          {
            MusEGlobal::config.warnInitPending = warn;
            // Save settings. Use simple version - do NOT set style or stylesheet, this has nothing to do with that.
            //MusEGlobal::muse->changeConfig(true);  // Save settings? No, wait till close.
          }
          if(rv != QDialog::Accepted)
          {
            if(MusEGlobal::config.midiSendInit)
              MusEGlobal::config.midiSendInit = false;
            //return;
          }
          else
          {
            if(!MusEGlobal::config.midiSendInit)
              MusEGlobal::config.midiSendInit = true;
          }
        }
      }
      
// We can either try to do it in one cycle with one message,
//  or by idling the sequencer (gaining safe access to all structures)
//  for as much time as we need.
// Here we COULD get away with the audio 'hiccup' that idling causes,
//  because it's unlikely someone would initialize during play...
// But no midi is processed, so let's switch this only if requiring
//  large numbers of init values causes a problem later...
#if 1         
      AudioMsg msg;
      msg.id = SEQM_INIT_DEVICES;
      msg.a = force;
      sendMessage(&msg, false);
#else      
      msgIdle(true); 
      initDevices(force);
      msgIdle(false); 
#endif
      
      }

//---------------------------------------------------------
//   panic
//---------------------------------------------------------

void Audio::msgPanic()
      {
      AudioMsg msg;
      msg.id = SEQM_PANIC;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   localOff
//---------------------------------------------------------

void Audio::msgLocalOff()
      {
      AudioMsg msg;
      msg.id = SEQM_MIDI_LOCAL_OFF;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgUpdateSoloStates
//---------------------------------------------------------

void Audio::msgUpdateSoloStates()
      {
      AudioMsg msg;
      msg.id    = SEQM_UPDATE_SOLO_STATES;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetAux
//---------------------------------------------------------

void Audio::msgSetAux(AudioTrack* track, int idx, double val)
      {
      AudioMsg msg;
      msg.id    = SEQM_SET_AUX;
      msg.snode = track;
      msg.ival  = idx;
      msg.dval  = val;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgPlayMidiEvent
//---------------------------------------------------------

void Audio::msgPlayMidiEvent(const MidiPlayEvent* event)
      {
      AudioMsg msg;
      msg.id = SEQM_PLAY_MIDI_EVENT;
      msg.p1 = event;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgSetHwCtrlState
//---------------------------------------------------------

void Audio::msgSetHwCtrlState(MidiPort* port, int ch, int ctrl, int val)
      {
      AudioMsg msg;
      msg.id = SEQM_SET_HW_CTRL_STATE;
      msg.p1 = port;
      msg.a = ch;
      msg.b = ctrl;
      msg.c = val;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgSetHwCtrlState
//---------------------------------------------------------

void Audio::msgSetHwCtrlStates(MidiPort* port, int ch, int ctrl, int val, int lastval)
      {
      AudioMsg msg;
      msg.id = SEQM_SET_HW_CTRL_STATES;
      msg.p1 = port;
      msg.a = ch;
      msg.b = ctrl;
      msg.c = val;
      msg.ival = lastval;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgSetTrackAutomationType
//---------------------------------------------------------

void Audio::msgSetTrackAutomationType(Track* track, int type)
{
      AudioMsg msg;
      msg.id = SEQM_SET_TRACK_AUTO_TYPE;
      msg.track = track;
      msg.ival = type;
      sendMessage(&msg, false);
}
      
//---------------------------------------------------------
//   msgSetSendMetronome
//---------------------------------------------------------

void Audio::msgSetSendMetronome(AudioTrack* track, bool b)
{
      AudioMsg msg;
      msg.id    = AUDIO_SET_SEND_METRONOME;
      msg.snode = track;
      msg.ival  = (int)b;
      sendMessage(&msg, false);
}

//---------------------------------------------------------
//   msgStartMidiLearn
//    Start learning midi 
//---------------------------------------------------------

void Audio::msgStartMidiLearn()
{
      AudioMsg msg;
      msg.id    = AUDIO_START_MIDI_LEARN;
      sendMessage(&msg, false);
}

//---------------------------------------------------------
//   msgBounce
//    start bounce operation
//---------------------------------------------------------

void Audio::msgBounce()
      {
      _bounce = true;
      if (!MusEGlobal::checkAudioDevice()) return;
      MusEGlobal::audioDevice->seekTransport(MusEGlobal::song->lPos());
      }

//---------------------------------------------------------
//   msgIdle
//---------------------------------------------------------

void Audio::msgIdle(bool on)
      {
      AudioMsg msg;
      msg.id = SEQM_IDLE;
      msg.a  = on;
      sendMessage(&msg, false);
      }

//---------------------------------------------------------
//   msgAudioWait
//---------------------------------------------------------

void Audio::msgAudioWait()
      {
      AudioMsg msg;
      msg.id     = AUDIO_WAIT;
      sendMsg(&msg);
      }

//---------------------------------------------------------
//   msgSetMidiDevice
//    to avoid timeouts in the RT-thread, setMidiDevice
//    is done in GUI context after setting the audio and midi threads
//    into idle mode
//---------------------------------------------------------

void Audio::msgSetMidiDevice(MidiPort* port, MidiDevice* device)
{
  MusECore::AudioMsg msg;
  msg.id = MusECore::SEQM_IDLE;
  msg.a  = true;
  //MusEGlobal::midiSeq->sendMsg(&msg);
  sendMsg(&msg); // Idle both audio and midi.

  port->setMidiDevice(device);

  msg.id = MusECore::SEQM_IDLE;
  msg.a  = false;
  //MusEGlobal::midiSeq->sendMsg(&msg);
  sendMsg(&msg); // Idle both audio and midi.
}

} // namespace MusECore
