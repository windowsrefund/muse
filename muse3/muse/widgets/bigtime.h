//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ./muse/widgets/bigtime.h $
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
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
#ifndef __BIGTIME_H__
#define __BIGTIME_H__

#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <cmath>

#include "type_defs.h"

class QCheckBox;
class QLabel;

namespace MusEGui {

class MusE;

class VerticalMetronomeWidget : public QWidget
{
   float metronomePosition;


protected:
    void paintEvent (QPaintEvent  *ev )
    {
      QRect rr(ev->rect());
      QPainter p(this);

      //p.fillRect(rr,Qt::yellow);
      int y = (rr.height() - (rr.height()*fabs(metronomePosition)))-1;
      if (metronomePosition > -0.05 and metronomePosition < 0.15) {
        p.setPen(Qt::red);
        p.drawLine(0,y-1,rr.width(),y-1);
      }
      else {
        p.setPen(Qt::yellow);
      }
      p.drawLine(0,y,rr.width(),y);
    }
public:
    VerticalMetronomeWidget(QWidget* parent) : QWidget(parent) {
      metronomePosition=0.0;
    }

    void setMetronome(float v)
    {
      metronomePosition = v;
      update();
    }
};


//---------------------------------------------------------
//   BigTime
//---------------------------------------------------------

class BigTime : public QWidget {
      Q_OBJECT
    
      bool tickmode;
      MusE* seq;
      
      unsigned _curPos;
      
      bool setString(unsigned);
      void updateValue();

      VerticalMetronomeWidget *metronome;
      QWidget *dwin;
      QCheckBox *fmtButton;
      QLabel *absTickLabel;
      QLabel *absFrameLabel;
      QLabel *barLabel, *beatLabel, *tickLabel,
             //*hourLabel, *minLabel, *secLabel, *frameLabel,
             *minLabel, *secLabel, *frameLabel, *subFrameLabel, 
             *sep1, *sep2, *sep3, *sep4, *sep5;
      
      //int oldbar, oldbeat, oldhour, oldmin, oldsec, oldframe;
      int oldbar, oldbeat, oldmin, oldsec, oldframe, oldsubframe;
      unsigned oldtick;
      unsigned oldAbsTick, oldAbsFrame;
      void setFgColor(QColor c);
      void setBgColor(QColor c);

   protected:
      virtual void resizeEvent(QResizeEvent*);
      virtual void closeEvent(QCloseEvent*);
      //void paintEvent (QPaintEvent  *event );

   public slots:
      void setPos(int, unsigned, bool);
      void configChanged();
      void songChanged(MusECore::SongChangedFlags_t);
      void fmtButtonToggled(bool);
   signals:
      void closed();

   public:
      BigTime(QWidget* parent);
      };

} // namespace MusEGui

#endif
