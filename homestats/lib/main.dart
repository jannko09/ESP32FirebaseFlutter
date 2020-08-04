import 'package:flutter/material.dart';
import 'package:flutter/cupertino.dart';
import 'package:HomeStats/screens/authController.dart';
import 'package:HomeStats/services/auth.dart';

void main() => runApp(MyApp());

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Home Stats',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: new RootPage(auth: new Auth()),
    );
  }
}
