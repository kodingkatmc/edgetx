/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "edgetx.h"

enum SensorFields {
  SENSOR_FIELD_NAME,
  SENSOR_FIELD_TYPE,
  SENSOR_FIELD_ID,
  SENSOR_FIELD_FORMULA = SENSOR_FIELD_ID,
  // SENSOR_FIELD_RECEIVER_NAME,
  SENSOR_FIELD_UNIT,
  SENSOR_FIELD_PRECISION,
  SENSOR_FIELD_PARAM1,
  SENSOR_FIELD_PARAM2,
  SENSOR_FIELD_PARAM3,
  SENSOR_FIELD_PARAM4,
  SENSOR_FIELD_AUTOOFFSET,
  SENSOR_FIELD_ONLYPOSITIVE,
  SENSOR_FIELD_FILTER,
  SENSOR_FIELD_PERSISTENT,
  SENSOR_FIELD_LOGS,
  SENSOR_FIELD_MAX
};

constexpr coord_t SENSOR_2ND_COLUMN = 12 * FW;
constexpr coord_t SENSOR_3RD_COLUMN = 17 * FW - 2;

void menuModelSensor(event_t event)
{
  TelemetrySensor * sensor = & g_model.telemetrySensors[s_currIdx];

  uint8_t old_editMode = s_editMode;
  
  SUBMENU(STR_MENUSENSOR, SENSOR_FIELD_MAX, {
    0, // Name
    0, // Type
    sensor->type == TELEM_TYPE_CALCULATED ? (uint8_t)0 : (uint8_t)1, // ID / Formula
    // sensor->type == TELEM_TYPE_CALCULATED ? HIDDEN_ROW : READONLY_ROW, // Receiver name
    ((sensor->type == TELEM_TYPE_CALCULATED && (sensor->formula == TELEM_FORMULA_DIST)) || sensor->isConfigurable() ? (uint8_t)0 : HIDDEN_ROW), // Unit
    (sensor->isPrecConfigurable()? (uint8_t)0 : HIDDEN_ROW), // Precision
    (sensor->unit >= UNIT_FIRST_VIRTUAL ? HIDDEN_ROW : (uint8_t)0), // Param1
    (sensor->unit == UNIT_GPS || sensor->unit == UNIT_DATETIME || sensor->unit == UNIT_CELLS || (sensor->type==TELEM_TYPE_CALCULATED && (sensor->formula==TELEM_FORMULA_CONSUMPTION || sensor->formula==TELEM_FORMULA_TOTALIZE)) ? HIDDEN_ROW : (uint8_t)0), // Param2
    (sensor->type == TELEM_TYPE_CALCULATED && sensor->formula < TELEM_FORMULA_MULTIPLY) ? (uint8_t)0 : HIDDEN_ROW, // Param3
    (sensor->type == TELEM_TYPE_CALCULATED && sensor->formula < TELEM_FORMULA_MULTIPLY) ? (uint8_t)0 : HIDDEN_ROW, // Param4
    (sensor->unit != UNIT_RPMS && sensor->isConfigurable() ? (uint8_t)0 : HIDDEN_ROW), // Auto offset
    (sensor->isConfigurable() ? (uint8_t)0 : HIDDEN_ROW), // Only positive
    (sensor->isConfigurable() ? (uint8_t)0 : HIDDEN_ROW), // Filter
    (sensor->type == TELEM_TYPE_CALCULATED ? (uint8_t)0 : HIDDEN_ROW), // Persistent
    0 // Logs
  });

  lcdDrawNumber(PSIZE(TR_MENUSENSOR)*FW+1, 0, s_currIdx+1, INVERS|LEFT);

  if (!isGPSSensor(s_currIdx+1))
    drawSensorCustomValue(SENSOR_2ND_COLUMN, 0, s_currIdx, getValue(MIXSRC_FIRST_TELEM+3*s_currIdx), LEFT);

  int8_t sub = menuVerticalPosition;

  for (uint8_t i=0; i<LCD_LINES-1; i++) {
    coord_t y = MENU_HEADER_HEIGHT + 1 + i*FH;
    uint8_t k = i + menuVerticalOffset;

    for (int j=0; j<k; j++) {
      if (mstate_tab[j+1] == HIDDEN_ROW)
        k++;
    }

    LcdFlags attr = (sub==k ? (s_editMode>0 ? BLINK|INVERS : INVERS) : 0);

    switch (k) {
      case SENSOR_FIELD_NAME:
        editSingleName(SENSOR_2ND_COLUMN, y, STR_NAME, sensor->label,
                       TELEM_LABEL_LEN, event, attr, old_editMode);
        break;

      case SENSOR_FIELD_TYPE:
        sensor->type = editChoice(SENSOR_2ND_COLUMN, y, STR_TYPE, STR_VSENSORTYPES, sensor->type, 0, 1, attr, event);
        if (attr && checkIncDec_Ret) {
          sensor->instance = 0;
          if (sensor->type == TELEM_TYPE_CALCULATED) {
            sensor->param = 0;
            sensor->autoOffset = 0;
            sensor->filter = 0;
          }
        }
        break;

      case SENSOR_FIELD_ID:
        if (sensor->type == TELEM_TYPE_CUSTOM) {
          lcdDrawTextAlignedLeft(y, STR_ID);
          lcdDrawHexNumber(SENSOR_2ND_COLUMN, y, sensor->id, LEFT|(menuHorizontalPosition==0 ? attr : 0));
          lcdDrawNumber(SENSOR_3RD_COLUMN, y, sensor->instance & 0x1F, LEFT|(menuHorizontalPosition==1 ? attr : 0));
          if (attr && s_editMode > 0) {
            switch (menuHorizontalPosition) {
              case 0:
                CHECK_INCDEC_MODELVAR_ZERO(event, sensor->id, 0xffff);
                break;

              case 1:
                uint8_t physID = sensor->instance & 0x1f;
                CHECK_INCDEC_MODELVAR_ZERO(event, physID, 0x1b);
                if (checkIncDec_Ret) {
                  sensor->instance = (sensor->instance & ~0x01f) + physID;
                }
                break;
            }
          }
        }
        else {
          sensor->formula = editChoice(SENSOR_2ND_COLUMN, y, STR_FORMULA, STR_VFORMULAS, sensor->formula, 0, TELEM_FORMULA_LAST, attr, event);
          if (attr && checkIncDec_Ret) {
            sensor->param = 0;
            if (sensor->formula == TELEM_FORMULA_CELL) {
              sensor->unit = UNIT_VOLTS;
              sensor->prec = 2;
            }
            else if (sensor->formula == TELEM_FORMULA_DIST) {
              sensor->unit = UNIT_DIST;
              sensor->prec = 0;
            }
            else if (sensor->formula == TELEM_FORMULA_CONSUMPTION) {
              sensor->unit = UNIT_MAH;
              sensor->prec = 0;
            }
          }
        }
        break;

      // TODO: this needs to be known from the sensor data alone!
//       case SENSOR_FIELD_RECEIVER_NAME:
//         lcdDrawTextAlignedLeft(y, STR_SOURCE);
//         if (telemetryProtocol == PROTOCOL_TELEMETRY_FRSKY_SPORT &&
//             sensor->frskyInstance.rxIndex != TELEMETRY_ENDPOINT_SPORT) {
//           drawReceiverName(SENSOR_2ND_COLUMN, y, sensor->frskyInstance.rxIndex >> 2,
//                            sensor->frskyInstance.rxIndex & 0x03, 0);
//         }
// #if defined(HARDWARE_INTERNAL_MODULE)
//         else if (isModuleUsingSport(INTERNAL_MODULE, g_model.moduleData[INTERNAL_MODULE].type)) {
//           // far from perfect
//           lcdDrawText(SENSOR_2ND_COLUMN, y, STR_INTERNAL_MODULE);
//         }
// #endif
//         else {
//           lcdDrawText(SENSOR_2ND_COLUMN, y, STR_EXTERNAL_MODULE);
//         }
//         break;

      case SENSOR_FIELD_UNIT:
        lcdDrawTextAlignedLeft(y, STR_UNIT);
        // TODO flash saving with editChoice where I copied those 2 lines?
        lcdDrawTextAtIndex(SENSOR_2ND_COLUMN, y, STR_VTELEMUNIT, sensor->unit, attr);
        if (attr) {
          CHECK_INCDEC_MODELVAR_ZERO(event, sensor->unit, UNIT_MAX);
          if (checkIncDec_Ret) {
            telemetryItems[s_currIdx].clear();
          }
        }
        break;

      case SENSOR_FIELD_PRECISION:
        sensor->prec = editChoice(SENSOR_2ND_COLUMN, y, STR_PRECISION, STR_VPREC, sensor->prec, 0, 2, attr, event);
        if (attr && checkIncDec_Ret) {
          telemetryItems[s_currIdx].clear();
        }
        break;

      case SENSOR_FIELD_PARAM1:
        if (sensor->type == TELEM_TYPE_CALCULATED) {
          if (sensor->formula == TELEM_FORMULA_CELL) {
            lcdDrawTextAlignedLeft(y, STR_CELLSENSOR);
            drawSource(SENSOR_2ND_COLUMN, y, sensor->cell.source ? MIXSRC_FIRST_TELEM+3*(sensor->cell.source-1) : 0, attr);
            if (attr) {
              sensor->cell.source = checkIncDec(event, sensor->cell.source, 0, MAX_TELEMETRY_SENSORS, EE_MODEL|NO_INCDEC_MARKS, isCellsSensor);
            }
            break;
          }
          else if (sensor->formula == TELEM_FORMULA_DIST) {
            lcdDrawTextAlignedLeft(y, STR_GPSSENSOR);
            drawSource(SENSOR_2ND_COLUMN, y, sensor->dist.gps ? MIXSRC_FIRST_TELEM+3*(sensor->dist.gps-1) : 0, attr);
            if (attr) {
              sensor->dist.gps = checkIncDec(event, sensor->dist.gps, 0, MAX_TELEMETRY_SENSORS, EE_MODEL|NO_INCDEC_MARKS, isGPSSensor);
            }
            break;
          }
          else if (sensor->formula == TELEM_FORMULA_CONSUMPTION) {
            lcdDrawTextAlignedLeft(y, STR_CURRENTSENSOR);
            drawSource(SENSOR_2ND_COLUMN, y, sensor->consumption.source ? MIXSRC_FIRST_TELEM+3*(sensor->consumption.source-1) : 0, attr);
            if (attr) {
              sensor->consumption.source = checkIncDec(event, sensor->consumption.source, 0, MAX_TELEMETRY_SENSORS, EE_MODEL|NO_INCDEC_MARKS, isCurrentSensor);
            }
            break;
          }
        }
        else {
          if (sensor->unit == UNIT_RPMS) {
            lcdDrawTextAlignedLeft(y, STR_BLADES);
            if (attr) CHECK_INCDEC_MODELVAR(event, sensor->custom.ratio, 1, 30000);
            lcdDrawNumber(SENSOR_2ND_COLUMN, y, sensor->custom.ratio, LEFT|attr);
            break;
          }
          else {
            lcdDrawTextAlignedLeft(y, STR_RATIO);
            if (attr) CHECK_INCDEC_MODELVAR(event, sensor->custom.ratio, 0, 30000);
            if (sensor->custom.ratio == 0) {
              lcdDrawChar(SENSOR_2ND_COLUMN, y, '-', attr);
            } else {  // Ratio + Ratio Percent
              uint32_t ratio = (sensor->custom.ratio * 1000) / 200;
              int ratioLen = countDigits(sensor->custom.ratio);
              int ratioPercLen = countDigits(ratio);

              int suffixOffset = 0;
              int ratioColAdj = 0;
              int ratioPercColAdj = 0;

              if (ratioLen <= 3) {
                ratioColAdj = 0;
              } else if (ratioLen <= 4) {
                ratioColAdj = (FWNUM * 1);
              } else if (ratioLen >= 5) {
                ratioColAdj = (FWNUM * 2);
              }

              if (ratioPercLen < 2) {
                ratioPercColAdj = 0;
                suffixOffset = (FWNUM * (ratioPercLen + 1)) + 2;
              } else if (ratioPercLen <= 4 ) {
                ratioPercColAdj = 0;
                suffixOffset = (FWNUM * ratioPercLen) + 2;
              } else if (ratioPercLen <= 5) {
                ratioColAdj += (FWNUM * 1); // move first column to maintain separation
                ratioPercColAdj = (FWNUM * 1);
                suffixOffset = (FWNUM * (ratioPercLen - 1)) + 2;
              } else if (ratioPercLen >= 6) {
                ratioColAdj += (FWNUM * 2); // move first column to maintain separation
                ratioPercColAdj = (FWNUM * 2);
                suffixOffset = (FWNUM * (ratioPercLen - 2)) + 2;
              }

              lcdDrawNumber(SENSOR_2ND_COLUMN - ratioColAdj, y,
                            sensor->custom.ratio, LEFT | attr | PREC1);
              lcdDrawNumber(SENSOR_3RD_COLUMN - ratioPercColAdj, y,
                            ratio, LEFT | PREC1);
              lcdDrawChar(SENSOR_3RD_COLUMN + suffixOffset, y, '%', 0);
            }
            break;
          }
        }
        // no break

      case SENSOR_FIELD_PARAM2:
        if (sensor->type == TELEM_TYPE_CALCULATED) {
          if (sensor->formula == TELEM_FORMULA_CELL) {
            sensor->cell.index = editChoice(SENSOR_2ND_COLUMN, y, STR_CELLINDEX, STR_VCELLINDEX, sensor->cell.index, TELEM_CELL_INDEX_LOWEST, TELEM_CELL_INDEX_LAST, attr, event);
            break;
          }
          else if (sensor->formula == TELEM_FORMULA_DIST) {
            lcdDrawTextAlignedLeft(y, STR_ALTSENSOR);
            drawSource(SENSOR_2ND_COLUMN, y, sensor->dist.alt ? MIXSRC_FIRST_TELEM+3*(sensor->dist.alt-1) : 0, attr);
            if (attr) {
              sensor->dist.alt = checkIncDec(event, sensor->dist.alt, 0, MAX_TELEMETRY_SENSORS, EE_MODEL|NO_INCDEC_MARKS, isAltSensor);
            }
            break;
          }
        }
        else if (sensor->unit == UNIT_RPMS) {
          lcdDrawTextAlignedLeft(y, STR_MULTIPLIER);
          if (attr) sensor->custom.offset = checkIncDec(event, sensor->custom.offset, 1, 30000, EE_MODEL|NO_INCDEC_MARKS|INCDEC_REP10);
          lcdDrawNumber(SENSOR_2ND_COLUMN, y, sensor->custom.offset, LEFT|attr);
          break;
        }
        else {
          lcdDrawTextAlignedLeft(y, STR_OFFSET);
          if (attr) CHECK_INCDEC_MODELVAR(event, sensor->custom.offset, -30000, +30000);
          if (sensor->prec > 0) attr |= (sensor->prec == 2 ? PREC2 : PREC1);
          lcdDrawNumber(SENSOR_2ND_COLUMN, y, sensor->custom.offset, LEFT|attr);
          break;
        }
        // no break

      case SENSOR_FIELD_PARAM3:
        // no break

      case SENSOR_FIELD_PARAM4:
      {
        drawStringWithIndex(0, y, STR_SOURCE, k-SENSOR_FIELD_PARAM1+1);
        int8_t * source = &sensor->calc.sources[k-SENSOR_FIELD_PARAM1];
        if (attr) {
          *source = checkIncDec(event, *source, -MAX_TELEMETRY_SENSORS, MAX_TELEMETRY_SENSORS, EE_MODEL|NO_INCDEC_MARKS, isSensorAvailable);
        }
        if (*source < 0) {
          lcdDrawChar(SENSOR_2ND_COLUMN, y, '-', attr);
          drawSource(lcdNextPos, y, MIXSRC_FIRST_TELEM+3*(-1-*source), attr);
        }
        else {
          drawSource(SENSOR_2ND_COLUMN, y, *source ? MIXSRC_FIRST_TELEM+3*(*source-1) : 0, attr);
        }
        break;
      }

      case SENSOR_FIELD_AUTOOFFSET:
        sensor->autoOffset = editCheckBox(sensor->autoOffset, SENSOR_2ND_COLUMN, y, STR_AUTOOFFSET, attr, event);
        break;

      case SENSOR_FIELD_ONLYPOSITIVE:
        sensor->onlyPositive = editCheckBox(sensor->onlyPositive, SENSOR_2ND_COLUMN, y, STR_ONLYPOSITIVE, attr, event);
        break;

      case SENSOR_FIELD_FILTER:
        sensor->filter = editCheckBox(sensor->filter, SENSOR_2ND_COLUMN, y, STR_FILTER, attr, event);
        break;

      case SENSOR_FIELD_PERSISTENT:
        sensor->persistent = editCheckBox(sensor->persistent, SENSOR_2ND_COLUMN, y, STR_PERSISTENT, attr, event, INDENT_WIDTH);
        if (checkIncDec_Ret && !sensor->persistent) {
          sensor->persistentValue = 0;
        }
        break;

      case SENSOR_FIELD_LOGS:
        sensor->logs = editCheckBox(sensor->logs, SENSOR_2ND_COLUMN, y, STR_LOGS, attr, event);
        if (attr && checkIncDec_Ret) {
          logsClose();
        }
        break;
    }
  }
}
