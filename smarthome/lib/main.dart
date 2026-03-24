// ignore_for_file: prefer_interpolation_to_compose_strings, unused_field, deprecated_member_use, use_super_parameters

import 'dart:convert';
import 'dart:io';
import 'package:flutter/material.dart';

void main() {
  runApp(const SmartHomeApp());
}

class SmartHomeApp extends StatelessWidget {
  const SmartHomeApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'IoT Controller',
      theme: ThemeData(
        brightness: Brightness.dark,
        primarySwatch: Colors.blue,
        scaffoldBackgroundColor: const Color(0xFF1E1E2C),
      ),
      home: const DashboardScreen(),
    );
  }
}

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({Key? key}) : super(key: key);

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  bool _isLightOn = false;
  String _energyConsumption = "0.0";
  String _systemLog = "System Ready. Waiting for user input...";
  String _liveGrid = ""; // متغير جديد لتخزين الـ 2D Grid

  Future<void> _toggleLight() async {
    try {
      setState(() {
        _systemLog = "Initiating TCP Handshake with 10.0.2.2:8080...";
      });

      Socket socket = await Socket.connect(
        '10.0.2.2',
        8080,
        timeout: const Duration(seconds: 5),
      );

      socket.write("TOGGLE_LIGHT");

      String responseBuffer = ""; // 1. متغير لتجميع الحزم المجزأة

      // 2. الاستماع للمجرى (Stream) بالكامل
      socket.listen(
        (List<int> event) {
          responseBuffer += utf8.decode(event); // تجميع الـ Bytes
        },
        onDone: () {
          // 3. التنفيذ والإغلاق فقط بعد انتهاء السيرفر من الإرسال
          _processBackendResponse(responseBuffer);
          socket.destroy();
        },
        onError: (error) {
          setState(() {
            _systemLog = "Stream Error: $error";
          });
          socket.destroy();
        },
      );
    } catch (e) {
      setState(() {
        _systemLog = "Network Error: $e";
      });
    }
  }

  // محرك فك تشفير الحزمة (Payload Parsing Engine)
  void _processBackendResponse(String data) {
    setState(() {
      _systemLog = "Payload received successfully.";

      // 1. تحليل حالة الجهاز
      if (data.contains("STATUS:ON")) {
        _isLightOn = true;
      } else if (data.contains("STATUS:OFF")) {
        _isLightOn = false;
      }

      // 2. استخراج الطاقة باستخدام Regex لضمان الدقة
      if (data.contains("ENERGY:")) {
        RegExp energyRegex = RegExp(r'ENERGY:([\d\.]+)');
        var match = energyRegex.firstMatch(data);
        if (match != null) {
          _energyConsumption =
              double.tryParse(match.group(1)!)?.toStringAsFixed(2) ?? "0.0";
        }
      }

      // 3. استخراج الـ 2D Grid
      if (data.contains("_GRID_START_")) {
        List<String> parts = data.split("_GRID_START_");
        if (parts.length > 1) {
          _liveGrid = parts[1].trim(); // حفظ الشبكة النصية
        }
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('CyberCore IoT Dashboard'),
        backgroundColor: const Color(0xFF292941),
        elevation: 0,
      ),
      body: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              "Connected Devices",
              style: TextStyle(
                fontSize: 22,
                fontWeight: FontWeight.bold,
                color: Colors.white,
              ),
            ),
            const SizedBox(height: 20),
            Container(
              padding: const EdgeInsets.all(20),
              decoration: BoxDecoration(
                color: const Color(0xFF292941),
                borderRadius: BorderRadius.circular(15),
                boxShadow: [
                  BoxShadow(
                    color: Colors.black.withOpacity(0.2),
                    blurRadius: 10,
                    offset: const Offset(0, 5),
                  ),
                ],
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Row(
                    children: [
                      Icon(
                        _isLightOn ? Icons.lightbulb : Icons.lightbulb_outline,
                        color: _isLightOn ? Colors.yellowAccent : Colors.grey,
                        size: 40,
                      ),
                      const SizedBox(width: 15),
                      Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          const Text(
                            "Living Room Light",
                            style: TextStyle(
                              fontSize: 18,
                              fontWeight: FontWeight.w600,
                              color: Colors.white,
                            ),
                          ),
                          Text(
                            "Energy: $_energyConsumption W",
                            style: const TextStyle(color: Colors.grey),
                          ),
                        ],
                      ),
                    ],
                  ),
                  Switch(
                    value: _isLightOn,
                    activeColor: Colors.yellowAccent,
                    onChanged: (value) {
                      _toggleLight();
                    },
                  ),
                ],
              ),
            ),
            const Spacer(),
            // واجهة الـ Terminal المدمجة لعرض الـ Grid
            Container(
              width: double.infinity,
              padding: const EdgeInsets.all(15),
              decoration: BoxDecoration(
                color: Colors.black,
                borderRadius: BorderRadius.circular(10),
                border: Border.all(color: Colors.greenAccent.withOpacity(0.5)),
              ),
              // استخدام SingleChildScrollView لتجنب خروج النص عن الشاشة
              child: SingleChildScrollView(
                scrollDirection: Axis.horizontal,
                child: Text(
                  // دمج رسائل النظام مع الـ 2D Grid
                  ">_ System Log: $_systemLog\n\n" +
                      (_liveGrid.isEmpty
                          ? ">_ Waiting for system sync...\n"
                          : _liveGrid),
                  style: const TextStyle(
                    color: Colors.greenAccent,
                    fontFamily: 'monospace',
                    fontSize: 10,
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
