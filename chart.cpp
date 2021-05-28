/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Charts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "chart.h"
#include <QtCharts/QAbstractAxis>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include <QtCore/QRandomGenerator>
#include <QtCore/QDebug>

Chart::Chart(QGraphicsItem *parent, Qt::WindowFlags wFlags):
    QChart(QChart::ChartTypeCartesian, parent, wFlags),
    axisX(new QValueAxis()),
    defaultAxisY(new QValueAxis()),
    cnt_x(0)
{
    // 初始化坐标轴
    addAxis(axisX.get(),Qt::AlignBottom);
    addAxis(defaultAxisY.get(),Qt::AlignLeft);
    points_per_frame = 4;
    axisX->setTickCount(points_per_frame);
    axisX->setRange(0, points_per_frame-1);
    axisX->setVisible(false);
    defaultAxisY->setRange(0, 10);
    curAxisY = defaultAxisY;
}

Chart::~Chart()
{

}

void Chart::createNewSerie(const QString& name, Qt::GlobalColor color, qreal min, qreal max)
{
    auto s = new QSplineSeries(this);
    auto y = new QValueAxis();

    QPen pen(color);
    pen.setWidth(3);
    s->setPen(pen);

    y->setRange(min, max);

    axisYs[name] = QSharedPointer<QValueAxis>(y);        // 新曲线未激活时不绑定任何Y轴
    addAxis(y, Qt::AlignLeft);
    y->setVisible(false);
    series[name] = QSharedPointer<QSplineSeries>(s);
    addSeries(s);
    s->setVisible(false);
    s->attachAxis(axisX.get());
}

void Chart::updateAxisY()
{
    curAxisY->setVisible(false);                     // 隐藏原先的轴
    for(auto &s : active_series)
    {
        s->detachAxis(curAxisY.get());               // 解绑
    }
    curAxisY = defaultAxisY;
    auto keys = active_series.keys();
    for(int i = 0; i < keys.size(); i++)                // 找到最大范围的纵轴
    {
        if (i == 0 || axisYs[keys[i]]->max() > curAxisY->max()) curAxisY = axisYs[keys[i]];
    }
    curAxisY->setVisible(true);
    for(auto &s : active_series)
    {
        s->attachAxis(curAxisY.get());
    }
}

void Chart::enableSerie(const QString& name)
{
    if (active_series.find(name) != active_series.end()) return;
    active_series.insert(name, series[name]);
    series[name]->setVisible(true);
    series[name]->attachAxis(curAxisY.get());
    updateAxisY();
}

void Chart::disableSerie(const QString& name)
{
    if (active_series.find(name) == active_series.end()) return;
    series[name]->setVisible(false);
    series[name]->detachAxis(curAxisY.get());
    active_series.remove(name);
    updateAxisY();
}

void Chart::addNewPoint(const QString& name, qreal value)
{
    series[name]->append(cnt_x, value);
}

void Chart::updateFrame()
{
    if (cnt_x >= points_per_frame)
    {
        axisX->setRange(cnt_x-points_per_frame+1, cnt_x);
    }
    cnt_x++;
}
