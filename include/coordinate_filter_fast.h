#pragma once
#include "coordinate_filter.h"
#include <memory>

/**
 * Schnelle Version des Koordinatenfilters für maximale Geschwindigkeit
 * Entfernt alle geschwindigkeitshemmenden Filtermechanismen
 */

// Factory-Funktion für schnellen Filter
std::unique_ptr<CoordinateFilter> createFastCoordinateFilter();
