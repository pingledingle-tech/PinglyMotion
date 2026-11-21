/*
 * PinglyMotion Script Lab - the world of the last lsl_run (step 5)
 *
 * Licensed under the GNU General Public License version 2 or later.
 */

#include "session.h"

namespace lsl
{

Session::Session(QObject* parent) : QObject(parent) {}

Session::~Session() = default;

void Session::set(std::unique_ptr<Sandbox> sandbox)
{
  m_sandbox = std::move(sandbox);
  emit changed();
}

void Session::clear()
{
  m_sandbox.reset();
  emit changed();
}

} // namespace lsl
