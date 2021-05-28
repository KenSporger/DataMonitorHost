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

#ifndef CHART_H
#define CHART_H

#include <QtCharts/QChart>
#include <QSharedPointer>

QT_CHARTS_BEGIN_NAMESPACE
class QSplineSeries;
class QValueAxis;
QT_CHARTS_END_NAMESPACE

QT_CHARTS_USE_NAMESPACE

//![1]
class Chart: public QChart
{
    Q_OBJECT
public:
    Chart(QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);
    virtual ~Chart();
    void updateAxisY();

public slots:
    void addNewPoint(const QString& name, qreal value);
    void updateFrame();
    void createNewSerie(const QString& name, Qt::GlobalColor color, qreal min, qreal max);
    void enableSerie(const QString& name);
    void disableSerie(const QString& name);

private:
    QMap<QString, QSharedPointer<QSplineSeries>> series; // 添加的曲线系列
    QMap<QString, QSharedPointer<QSplineSeries>> active_series; // 激活显式的曲线系列
    QMap<QString, QPair<qreal, qreal>> yRange; // 每个系列对应的纵轴范围
    QStringList titles;
    QValueAxis* axisX; // 横轴
    QValueAxis* axisY; // 纵轴
    qreal default_maxY;
    qreal default_minY;
    uint32_t cnt_x;
    uint32_t points_per_frame;
};
//![1]

#endif /* CHART_H */
