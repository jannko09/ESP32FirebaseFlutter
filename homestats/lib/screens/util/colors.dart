import 'package:flutter/material.dart';

class HexToColor extends Color {
  static _hexToColor(String code) {
    try {
      return int.parse(code.substring(1, 7), radix: 16) + 0xFF000000;
    } catch (e) {
      print(e);
      return 0xFF000000;
    }
  }

  HexToColor(final String code) : super(_hexToColor(code));


}
