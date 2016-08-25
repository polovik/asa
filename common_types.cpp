#include <QDebug>
#include "common_types.h"

ToneWaveForm::ToneWaveForm()
{
    m_id = WAVE_UNKNOWN;
}

ToneWaveForm::ToneWaveForm(Id id)
{
    setId(id);
}

ToneWaveForm::ToneWaveForm(QString name)
{
    if (name == "sine") {
        m_id = WAVE_SINE;
    } else if (name == "square") {
        m_id = WAVE_SQUARE;
    } else if (name == "sawtooth") {
        m_id = WAVE_SAWTOOTH;
    } else if (name == "triangle") {
        m_id = WAVE_TRIANGLE;
    } else {
        qWarning() << "Invalid format of waveform's name:" << name;
        Q_ASSERT(false);
        m_id = WAVE_UNKNOWN;
    }
}

ToneWaveForm::Id ToneWaveForm::id()
{
    return m_id;
}

void ToneWaveForm::setId(Id id)
{
    if ((id <= WAVE_UNKNOWN) || (id >= WAVE_LAST)) {
        qWarning() << "Invalid signal waveform id:" << id;
        Q_ASSERT(false);
        id = WAVE_UNKNOWN;
    }
    m_id = id;
}

QString ToneWaveForm::getName() const
{
    switch (m_id) {
    case WAVE_UNKNOWN:
        return "unknown";
    case WAVE_SINE:
        return "sine";
    case WAVE_SQUARE:
        return "square";
    case WAVE_SAWTOOTH:
        return "sawtooth";
    case WAVE_TRIANGLE:
        return "triangle";
    case WAVE_LAST:
        return "-----";
    default:
        qWarning() << "Invalid tone's waveform id:" << m_id;
        Q_ASSERT(false);
        return "sine";
    }
}

QString ToneWaveForm::getName(Id id)
{
    ToneWaveForm wave(id);
    return wave.getName();
}

QDebug operator<<(QDebug d, const ToneWaveForm &tone)
{
    d << tone.getName();
    return d;
}
