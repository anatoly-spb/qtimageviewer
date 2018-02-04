#ifndef RXEVENTLOOP_H
#define RXEVENTLOOP_H

#include <QThread>
#include <QTimer>
#include <QtDebug>

#include "rxcpp/rx.hpp"

#include <chrono>

/**
 * @brief The RxEventLoopAdapter class
 * Класс адаптера rxcpp run loop для работы с qt event loop
 */
class RxEventLoopAdapter {
public:
    /**
     * @brief rxRunLoopAdaptedToEventLoop возвращает rxcpp run loop, адаптированный к работе через qt event loop
     * @return rxcpp run loop
     */
    static rxcpp::schedulers::run_loop& runLoop()
    {
        static thread_local RxEventLoopAdapter adapter;
        return adapter.rxRunLoop();
    }

private:
    /**
     * @brief RxEventLoopAdapter конструирует адаптер
     */
    RxEventLoopAdapter()
    {
        qDebug() << "RxEventLoopAdapter::RxEventLoopAdapter()";
        // просим rxcpp run_loop сообщать о планируемых событиях
        m_rxRunLoop.set_notify_earlier_wakeup([this](std::chrono::steady_clock::time_point when) {
            const auto dispatchTimeOut = nextDispatchTimeOut(when);
            if (!m_timer.isActive() || dispatchTimeOut.count() < m_timer.remainingTime()) {
                m_timer.start(dispatchTimeOut.count());
            }
        });
        m_timer.setSingleShot(true);
        m_timer.setTimerType(Qt::PreciseTimer);
        QTimer::connect(&m_timer, &QTimer::timeout, [this]() {
            // пока есть запланированные события выполняем диспетчеризацию
            while (!m_rxRunLoop.empty() && m_rxRunLoop.peek().when < m_rxRunLoop.now()) {
                m_rxRunLoop.dispatch();
            }
            // если в очереди остались запланированные на будущее события,
            // "заводим" будильник на диспетчеризацию ближайшего
            if (!m_rxRunLoop.empty()) {
                const auto dispatcheTimeOut = nextDispatchTimeOut(m_rxRunLoop.peek().when);
                m_timer.start(static_cast<int>(dispatcheTimeOut.count()));
            }
        });
    }

    ~RxEventLoopAdapter()
    {
        qDebug() << "RxEventLoopAdapter::~RxEventLoopAdapter()";
        m_rxRunLoop.set_notify_earlier_wakeup([](std::chrono::steady_clock::time_point) {});
    }

    /**
     * @brief rxRunLoop вернуть адаптированный для работы с qt event loop rxcpp run loop
     * @return адаптированный rxcpp run loop
     */
    rxcpp::schedulers::run_loop& rxRunLoop()
    {
        return m_rxRunLoop;
    }

    /**
     * Адаптировать duration, исходя из точности To
     */
    template <class To, class Rep, class Period>
    static To durationCeil(const std::chrono::duration<Rep, Period>& duration)
    {
        const auto as_To = std::chrono::duration_cast<To>(duration);
        return (as_To < duration) ? (as_To + To{ 1 }) : as_To;
    }

    /**
     * @brief nextDispatcheTimeOut вернуть таймаут, через который необходимы выполнить диспетчеризацию, запланированную на момент времени when
     * @param when момент следующей диспетчеризации
     * @return таймаут, через который необходимы выполнить диспетчеризацию
     */
    std::chrono::milliseconds nextDispatchTimeOut(rxcpp::schedulers::run_loop::clock_type::time_point const& when) const
    {
        return durationCeil<std::chrono::milliseconds>(when - m_rxRunLoop.now());
    }

private:
    /**
     * @brief m_rxRunLoop адаптируемый rxcpp run loop
     */
    rxcpp::schedulers::run_loop m_rxRunLoop;
    /**
     * @brief m_timer таймер, через который будет осуществляться активация диспетчеризации rxcpp run loop
     */
    QTimer m_timer;
};

#endif // RXEVENTLOOP_H
