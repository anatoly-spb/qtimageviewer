#ifndef RXEVENTLOOP_H
#define RXEVENTLOOP_H

#include <QThread>
#include <QTimer>

#include "rxcpp/rx.hpp"

/**
 * @brief The RxEventLoopAdapter class is rxcpp run loop adapter adapted to execution in qt event loop
 * @see https://github.com/tetsurom/rxqt/blob/master/include/rxqt_run_loop.hpp
 */
class RxEventLoopAdapter {
public:
    /**
     * @brief rxRunLoopAdaptedToEventLoop returns adapted rxcpp run loop
     * @return rxcpp run loop
     */
    static rxcpp::schedulers::run_loop& runLoop()
    {
        static thread_local RxEventLoopAdapter adapter;
        return adapter.rxRunLoop();
    }

private:
    /**
     * @brief RxEventLoopAdapter constructs adapter object
     */
    RxEventLoopAdapter()
        : m_ownerThreadId(QThread::currentThreadId())
    {
        // ask to rxcpp run_loop to notify about sheduled events
        m_rxRunLoop.set_notify_earlier_wakeup([this](auto&& when) {
            const auto sheduledTimeOut = sheduledTimeOutFor(when);
            if (!m_timer.isActive() || sheduledTimeOut.count() < m_timer.remainingTime()) {
                int dispatched = static_cast<int>(sheduledTimeOut.count());
                if (dispatched < 0) {
                    dispatched = 0;
                }
                if (m_ownerThreadId == QThread::currentThreadId()) {
                    m_timer.start(dispatched);
                } else {
                    QMetaObject::invokeMethod(&m_timer, "start", Qt::QueuedConnection, Q_ARG(int, dispatched));
                }
            }
        });
        m_timer.setSingleShot(true);
        m_timer.setTimerType(Qt::PreciseTimer);
        QTimer::connect(&m_timer, &QTimer::timeout, [this]() {
            while (!m_rxRunLoop.empty() && m_rxRunLoop.peek().when < m_rxRunLoop.now()) {
                m_rxRunLoop.dispatch();
            }
            // if there are future events, shedule them
            if (!m_rxRunLoop.empty()) {
                const auto sheduledTimeOut = sheduledTimeOutFor(m_rxRunLoop.peek().when);
                m_timer.start(static_cast<int>(sheduledTimeOut.count()));
            }
        });
    }

    /**
      * @brief ~RxEventLoopAdapter destructs adapter
      */
    ~RxEventLoopAdapter()
    {
        m_rxRunLoop.set_notify_earlier_wakeup([](auto&&) {});
    }

    /**
     * @brief rxRunLoop return rxcpp run loop
     * @return адаптированный rxcpp run loop
     */
    rxcpp::schedulers::run_loop& rxRunLoop()
    {
        return m_rxRunLoop;
    }

    /**
     * Ceil duration
     */
    template <class To, class Rep, class Period>
    static To durationCeil(const std::chrono::duration<Rep, Period>& duration)
    {
        const auto to = std::chrono::duration_cast<To>(duration);
        return (to < duration) ? (to + To{ 1 }) : to;
    }

    /**
     * @brief nextDispatcheTimeOut must return timeout interval for the sheduled event
     * @param when time moment when event is scheduled
     * @return timeout
     */
    std::chrono::milliseconds sheduledTimeOutFor(rxcpp::schedulers::run_loop::clock_type::time_point const& when) const
    {
        return durationCeil<std::chrono::milliseconds>(when - m_rxRunLoop.now());
    }

private:
    /**
     * @brief m_ownerThreadId thread id where this object was created
     */
    Qt::HANDLE m_ownerThreadId;
    /**
     * @brief m_rxRunLoop rxcpp run loop
     */
    rxcpp::schedulers::run_loop m_rxRunLoop;
    /**
     * @brief m_timer dispatch timer
     */
    QTimer m_timer;
};

#endif // RXEVENTLOOP_H
