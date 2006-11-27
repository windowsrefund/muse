//=========================================================
//  MusE
//  Linux Music Editor
//
//  vamgui.c
//	This is a simple GUI implemented with QT for
//	vam software synthesizer.
//	(Many) parts of this file was taken from Werner Schweer's GUI
//	for his organ soft synth.
//
//  (C) Copyright 2002 Jotsif Lindman H�nlund (jotsif@linux.nu)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA or point your web browser to http://www.gnu.org.
//=========================================================

#include "vamgui.h"
#include "vam.h"

#include "al/xml.h"
using AL::Xml;

#include "muse/midi.h"
#include "muse/midictrl.h"

//---------------------------------------------------------
//   Preset
//---------------------------------------------------------

struct Preset {
	QString name;
	int ctrl[NUM_CONTROLLER];
	void readConfiguration(QDomNode);
	void readControl(QDomNode);
	void writeConfiguration(Xml& xml);
      };

std::list<Preset> presets;
typedef std::list<Preset>::iterator iPreset;

QString museProject;
QString museGlobalShare;
QString museUser;

QString instanceName;
//static const char* presetFileTypes[] = {
//      "Presets (*.pre)",
//      0
//      };

//---------------------------------------------------------
//   readControl
//---------------------------------------------------------

void Preset::readControl(QDomNode)
{
#if 0
	int idx = 0;
	int val = 0;
	for (;;) {
		Xml::Token token(xml.parse());
		const QString& tag(xml.s1());
		switch (token) {
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				xml.unknown("control");
				break;
			case Xml::Attribut:
				if (tag == "idx") {
					idx = xml.s2().toInt();
						if (idx >= NUM_CONTROLLER)
							idx = 0;
				}
				else if (tag == "val")
					val = xml.s2().toInt();
				break;
			case Xml::TagEnd:
				if (tag == "control") {
					ctrl[idx] = val;
					return;
				}
			default:
                    		break;
		}
	}
#endif
}

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void Preset::readConfiguration(QDomNode /*node*/)
{
#if 0
	for (;;) {
		Xml::Token token(xml.parse());
		const QString& tag(xml.s1());
		switch (token) {
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "control")
					readControl(xml);
				else
					xml.unknown("preset");
				break;
			case Xml::Attribut:
				if (tag == "name")
					name = xml.s2();
				break;
			case Xml::TagEnd:
				if (tag == "preset")
					return;
			default:
				break;
		}
	}
#endif
}

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void Preset::writeConfiguration(Xml& xml)
{
	xml.stag("preset name=\"%s\"", name.toLatin1().data());
	for (int i = 0; i < NUM_CONTROLLER; ++i) {
		xml.tagE("control idx=\"%d\" val=\"%d\"", i, ctrl[i]);
	}
	xml.etag("preset");
}

//---------------------------------------------------------
//   VAMGui
//---------------------------------------------------------

VAMGui::VAMGui()
//   : QWidget(0, "vamgui", Qt::WType_TopLevel),
   : QWidget(0),
	MessGui()
{
   setupUi(this);
      QSocketNotifier* s = new QSocketNotifier(readFd, QSocketNotifier::Read);
      connect(s, SIGNAL(activated(int)), SLOT(readMessage(int)));

	dctrl[DCO1_PITCHMOD] = SynthGuiCtrl(PitchModS, LCDNumber1,  SynthGuiCtrl::SLIDER);
	dctrl[DCO1_WAVEFORM] = SynthGuiCtrl(Waveform, 0, SynthGuiCtrl::COMBOBOX);
	dctrl[DCO1_FM] = SynthGuiCtrl(FMS, LCDNumber1_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_PWM] = SynthGuiCtrl(PWMS, LCDNumber1_3, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_ATTACK] = SynthGuiCtrl(AttackS, LCDNumber1_3_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_DECAY] = SynthGuiCtrl(DecayS, LCDNumber1_3_2_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_SUSTAIN] = SynthGuiCtrl(SustainS, LCDNumber1_3_2_3, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_RELEASE] = SynthGuiCtrl(ReleaseS, LCDNumber1_3_2_4, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_PITCHMOD] = SynthGuiCtrl(PitchModS2, LCDNumber1_4, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_WAVEFORM] = SynthGuiCtrl(Waveform2, 0, SynthGuiCtrl::COMBOBOX);
	dctrl[DCO2_FM] = SynthGuiCtrl(FMS2, LCDNumber1_2_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_PWM] = SynthGuiCtrl(PWMS2, LCDNumber1_3_3, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_ATTACK] = SynthGuiCtrl(AttackS2, LCDNumber1_3_2_5, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_DECAY] = SynthGuiCtrl(DecayS2, LCDNumber1_3_2_2_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_SUSTAIN] = SynthGuiCtrl(SustainS2, LCDNumber1_3_2_3_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_RELEASE] = SynthGuiCtrl(ReleaseS2, LCDNumber1_3_2_4_2, SynthGuiCtrl::SLIDER);
	dctrl[LFO_FREQ] = SynthGuiCtrl(FreqS, LCDNumber1_5, SynthGuiCtrl::SLIDER);
	dctrl[LFO_WAVEFORM] = SynthGuiCtrl(Waveform2_2, 0, SynthGuiCtrl::COMBOBOX);
	dctrl[FILT_ENV_MOD] = SynthGuiCtrl(EnvModS, LCDNumber1_5_3, SynthGuiCtrl::SLIDER);
	dctrl[FILT_KEYTRACK] = SynthGuiCtrl(KeyTrack, 0, SynthGuiCtrl::SWITCH);
	dctrl[FILT_RES] = SynthGuiCtrl(ResS, LCDNumber1_5_5,  SynthGuiCtrl::SLIDER);
	dctrl[FILT_ATTACK] = SynthGuiCtrl(AttackS3, LCDNumber1_3_2_5_2, SynthGuiCtrl::SLIDER);
	dctrl[FILT_DECAY] = SynthGuiCtrl(DecayS3, LCDNumber1_3_2_2_2_2, SynthGuiCtrl::SLIDER);
	dctrl[FILT_SUSTAIN] = SynthGuiCtrl(SustainS3, LCDNumber1_3_2_3_2_2, SynthGuiCtrl::SLIDER);
	dctrl[FILT_RELEASE] = SynthGuiCtrl(ReleaseS3, LCDNumber1_3_2_4_2_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO2ON] = SynthGuiCtrl(DCO2On, 0, SynthGuiCtrl::SWITCH);
	dctrl[FILT_INVERT] = SynthGuiCtrl(FilterInvert, 0, SynthGuiCtrl::SWITCH);
	dctrl[FILT_CUTOFF] = SynthGuiCtrl(CutoffS, LCDNumber1_5_5_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_DETUNE] = SynthGuiCtrl(DetuneS, LCDNumber1_6, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_DETUNE] = SynthGuiCtrl(DetuneS2, LCDNumber1_6_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_PW] = SynthGuiCtrl(PWS, LCDNumber1_2_3, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_PW] = SynthGuiCtrl(PWS2, LCDNumber1_2_4, SynthGuiCtrl::SLIDER);


	map = new QSignalMapper(this);
	for (int i = 0; i < NUM_CONTROLLER; ++i) {
		map->setMapping(dctrl[i].editor, i);
		if (dctrl[i].type == SynthGuiCtrl::SLIDER)
			connect((QSlider*)(dctrl[i].editor), SIGNAL(valueChanged(int)), map, SLOT(map()));
		else if (dctrl[i].type == SynthGuiCtrl::COMBOBOX)
			connect((QComboBox*)(dctrl[i].editor), SIGNAL(activated(int)), map, SLOT(map()));
		else if (dctrl[i].type == SynthGuiCtrl::SWITCH)
			connect((QCheckBox*)(dctrl[i].editor), SIGNAL(toggled(bool)), map, SLOT(map()));
	}
	connect(map, SIGNAL(mapped(int)), this, SLOT(ctrlChanged(int)));

	connect(presetList, SIGNAL(clicked(QListWidgetItem*)),
		this, SLOT(presetClicked(QListWidgetItem*)));
	// presetNameEdit
	connect(presetSet,   SIGNAL(clicked()), this, SLOT(setPreset()));
	connect(savePresets, SIGNAL(clicked()), this, SLOT(savePresetsPressed()));
	connect(loadPresets, SIGNAL(clicked()), this, SLOT(loadPresetsPressed()));
	connect(deletePreset, SIGNAL(clicked()), this, SLOT(deletePresetPressed()));
	connect(savePresetsToFile, SIGNAL(clicked()), this, SLOT(savePresetsToFilePressed()));

	ctrlHi = 0;
	ctrlLo = 0;
	dataHi = 0;
	dataLo = 0;
      }

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void VAMGui::ctrlChanged(int idx)
      {
	SynthGuiCtrl* ctrl = &dctrl[idx];
	int val = 0;
	if (ctrl->type == SynthGuiCtrl::SLIDER) {
    		QSlider* slider = (QSlider*)(ctrl->editor);
		int max = slider->maximum();
		val = (slider->value() * 16383 + max/2) / max;
	      }
	else if (ctrl->type == SynthGuiCtrl::COMBOBOX) {
		val = ((QComboBox*)(ctrl->editor))->currentIndex();
	      }
	else if (ctrl->type == SynthGuiCtrl::SWITCH) {
		val = ((QCheckBox*)(ctrl->editor))->isChecked();
	      }
      sendController(0, idx + CTRL_RPN14_OFFSET, val);
      }

//---------------------------------------------------------
//   presetClicked
//---------------------------------------------------------

void VAMGui::presetClicked(QListWidgetItem* item)
{
	if (item == 0)
		return;
	presetNameEdit->setText(item->text());
	Preset* preset = 0;
	for (iPreset i = presets.begin(); i != presets.end(); ++i) {
		if (i->name == item->text()) {
			preset = &*i;
			break;
		}
	}
	activatePreset(preset);
}

//---------------------------------------------------------
//   setPreset
//---------------------------------------------------------

void VAMGui::activatePreset(Preset* preset)
{
	if (preset == 0) {
		fprintf(stderr, "internal error 1\n");
		exit(-1);
	}
	for (unsigned int i = 0; i < sizeof(dctrl)/sizeof(*dctrl); ++i) {
		setParam(i, preset->ctrl[i]);
		ctrlChanged(i);
	}
}

//---------------------------------------------------------
//   setPreset
//---------------------------------------------------------

void VAMGui::setPreset()
{
	if (presetNameEdit->text().isEmpty())
		return;
	for (iPreset i = presets.begin(); i != presets.end(); ++i) {
		if (i->name == presetNameEdit->text()) {
			setPreset(&*i);
			return;
		}
	}
	addNewPreset(presetNameEdit->text());
}

//---------------------------------------------------------
//   addNewPreset
//---------------------------------------------------------

void VAMGui::addNewPreset(const QString& name)
{
	Preset p;
	p.name = name;
	setPreset(&p);
	presets.push_back(p);
	presetList->addItem(name);
}

//---------------------------------------------------------
//   deleteNamedPreset
//---------------------------------------------------------
void VAMGui::deleteNamedPreset(const QString& name)
{
	QListWidgetItem * item = presetList->findItems(name,Qt::MatchExactly).at(0);
	if (!item) {
		fprintf(stderr, "%s: Could not find preset!\n", __FUNCTION__);
		return;
	}
	presetList->clearSelection();
      delete item;
//	int index = presetList->row(item);
//	presetList->removeItem(index);
	for (iPreset i = presets.begin(); i != presets.end(); ++i) {
		if (i->name == name) {
			presets.erase(i);
			break;
		}
	}
}


//---------------------------------------------------------
//   setPreset
//---------------------------------------------------------

void VAMGui::setPreset(Preset* preset)
{
	for (unsigned int i = 0; i < NUM_CONTROLLER; ++i) {
		int val = 0;
		SynthGuiCtrl* ctrl = &dctrl[i];
		if (ctrl->type == SynthGuiCtrl::SLIDER) {
			QSlider* slider = (QSlider*)(ctrl->editor);
			int max = slider->maximum();
			val = (slider->value() * 16383 + max/2) / max;
		}
		else if (ctrl->type == SynthGuiCtrl::COMBOBOX) {
			val = ((QComboBox*)(ctrl->editor))->currentIndex();
		}
		else if (ctrl->type == SynthGuiCtrl::SWITCH) {
			val = ((QCheckBox*)(ctrl->editor))->isChecked();
		}

		preset->ctrl[i] = val;
	}
	//
	// send sysex to synti
	//
#if 0
	putchar(0xf0);
	putchar(0x7c);    // mess
	putchar(0x2);     // vam
	putchar(0x3);     // setPreset
	const char* name = preset->name.toLatin1().data();
	while (*name)
		putchar(*name++ & 0x7f);
	putchar(0);
	for (int i = 0; i < NUM_CONTROLLER; ++i) {
		putchar(i);
		putchar(preset->ctrl[i]);
	}
	putchar(0xf7);
#endif
}

//---------------------------------------------------------
//   setParam
//    set param in gui
//    val -- midi value 0 - 16383
//---------------------------------------------------------

void VAMGui::setParam(int param, int val)
      {
	if (param >= int(sizeof(dctrl)/sizeof(*dctrl))) {
		fprintf(stderr, "vam: set unknown parameter 0x%x to 0x%x\n", param, val);
		return;
	      }
	SynthGuiCtrl* ctrl = &dctrl[param];
	ctrl->editor->blockSignals(true);
	if (ctrl->type == SynthGuiCtrl::SLIDER) {
		QSlider* slider = (QSlider*)(ctrl->editor);
		int max = slider->maximum();
		if(val < 0) val = (val * max + 8191) / 16383 - 1;
		else val = (val * max + 8191) / 16383;
		
		slider->setValue(val);
		if (ctrl->label)
			((QLCDNumber*)(ctrl->label))->display(val);
	      }
	else if (ctrl->type == SynthGuiCtrl::COMBOBOX) {
		((QComboBox*)(ctrl->editor))->setCurrentIndex(val);
	      }
	else if (ctrl->type == SynthGuiCtrl::SWITCH) {
		((QCheckBox*)(ctrl->editor))->setChecked(val);
	      }
	ctrl->editor->blockSignals(false);
      }

//---------------------------------------------------------
//   sysexReceived
//---------------------------------------------------------

void VAMGui::sysexReceived(const unsigned char* data, int len)
{
	if (len >= 4) {
		//---------------------------------------------
		//  MusE Soft Synth
		//---------------------------------------------

		if (data[0] == 0x7c) {
			if (data[1] == 2) {     // vam
				if (data[2] == 2) {        // parameter response
					if (len != 6) {
						fprintf(stderr, "vam gui: bad sysEx len\n");
						return;
					}
					int val = data[4] + (data[5]<<7);
					switch(data[3])
					{
						case DCO1_PITCHMOD:
						case DCO2_PITCHMOD:
						case DCO1_DETUNE:
						case DCO2_DETUNE:
							setParam(data[3], ((val + 1) * 2) - 16383);
							break;
						default:
							setParam(data[3], val);
							break;
					}
					return;
				}
				else if (data[2] == 1) {  // param request
					return;
				}
			}
		}
	}
	fprintf(stderr, "vam gui: unknown sysex received, len %d:\n", len);
	for (int i = 0; i < len; ++i)
		fprintf(stderr, "%02x ", data[i]);
	fprintf(stderr, "\n");
}

//---------------------------------------------------------
//   processEvent
//---------------------------------------------------------

void VAMGui::processEvent(const MidiEvent& ev)
      {
      if (ev.type() == ME_CONTROLLER)
            setParam(ev.dataA() & 0xfff, ev.dataB());
      else if (ev.type() == ME_SYSEX)
            sysexReceived(ev.data(), ev.len())
            ;
      else
            printf("VAMGui::illegal event type received\n");
      }

//---------------------------------------------------------
//   loadPresetsPressed
//---------------------------------------------------------

void VAMGui::loadPresetsPressed()
{
#if 0   // TODO
	QString iname;
	QString s(getenv("HOME"));
	QString fn = getOpenFileName(s, presetFileTypes, this,
         tr("MusE: Load VAM Presets"), 0);
	if (fn.isEmpty())
		return;
	bool popenFlag;
	FILE* f = fileOpen(this, fn, QString(".pre"), "r", popenFlag, true);
	if (f == 0)
		return;
	presets.clear();
	presetList->clear();

	Xml xml(f);
	int mode = 0;
	for (;;) {
		Xml::Token token = xml.parse();
		QString tag = xml.s1();
		switch (token) {
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (mode == 0 && tag == "muse")
					mode = 1;
//				else if (mode == 1 && tag == "instrument")
//					mode = 2;

				else if (mode == 2  && tag == "preset") {
					Preset preset;
					preset.readConfiguration(xml);
					presets.push_back(preset);
					presetList->insertItem(preset.name);
				}
				else if(mode != 1)
					xml.unknown("SynthPreset");
				break;
			case Xml::Attribut:
				if(mode == 1 && tag == "iname") {
//					fprintf(stderr, "%s\n", xml.s2().toLatin1().data());
					if(xml.s2() != "vam-1.0")
						return;
					else mode = 2;
				}
                    		break;
			case Xml::TagEnd:
				if (tag == "muse")
				goto ende;
			default:
				break;
		}
	}
ende:
	if (popenFlag)
		pclose(f);
	else
		fclose(f);

	if (presetFileName) delete presetFileName;
	presetFileName = new QString(fn);
	QString dots ("...");
	fileName->setText(fn.right(32).insert(0, dots));

	if (presets.empty())
		return;
	Preset preset = presets.front();
	activatePreset(&preset);
#endif
}

//---------------------------------------------------------
//   doSavePresets
//---------------------------------------------------------
void VAMGui::doSavePresets(const QString& /*fn*/, bool /*showWarning*/)
{
#if 0
	bool popenFlag;
	FILE* f = fileOpen(this, fn, QString(".pre"), "w", popenFlag, false, showWarning);
	if (f == 0)
		return;
	Xml xml(f);
	xml.header();
	xml.tag(0, "muse version=\"1.0\"");
	xml.tag(0, "instrument iname=\"vam-1.0\" /");

	for (iPreset i = presets.begin(); i != presets.end(); ++i)
		i->writeConfiguration(xml, 1);

	xml.tag(1, "/muse");

	if (popenFlag)
		pclose(f);
	else
		fclose(f);
#endif
}

//---------------------------------------------------------
//   savePresetsPressed
//---------------------------------------------------------

void VAMGui::savePresetsPressed()
{
#if 0 // TODO
	QString s(getenv("MUSE"));
	QString fn = getSaveFileName(s, presetFileTypes, this,
         tr("MusE: Save VAM Presets"));
	if (fn.isEmpty())
		return;
	doSavePresets (fn, true);
#endif
}


//---------------------------------------------------------
//   savePresetsToFilePressed
//---------------------------------------------------------

void VAMGui::savePresetsToFilePressed()
{
	if (!presetFileName) return;
	doSavePresets (*presetFileName, false);
}

//---------------------------------------------------------
//   deletePresetPressed
//---------------------------------------------------------

void VAMGui::deletePresetPressed()
{
	deleteNamedPreset (presetList->currentItem()->text());
}

//---------------------------------------------------------
//   readMessage
//---------------------------------------------------------

void VAMGui::readMessage(int)
      {
      MessGui::readMessage();
      }

