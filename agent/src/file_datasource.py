from csv import reader
from datetime import datetime
from domain.accelerometer import Accelerometer
from domain.gps import Gps
from domain.aggregated_data import AggregatedData
import config
import os

class FileDatasource:
    def __init__(
        self,
        accelerometer_filename: str,
        gps_filename: str,
    ) -> None:
        self.accel_filename = accelerometer_filename
        self.gps_filename = gps_filename
        self.reading = False
        self.accel_line = 0
        self.gps_line = 0

        with open(self.gps_filename) as file:
            lines = [line.rstrip() for line in file]
            lines = lines[1:]
            self.gps_lines = lines

        with open(self.accel_filename) as file:
            lines = [line.rstrip() for line in file]
            lines = lines[1:]
            self.accel_lines = lines

    def read(self) -> AggregatedData:
        """Метод повертає дані отримані з датчиків"""
        data = AggregatedData(
            Accelerometer(1, 2, 3),
            Gps(4, 5),
            datetime.now(),
            config.USER_ID,
        )

        if self.reading == True:
            if self.gps_line > len(self.gps_lines) - 1:
                self.gps_line = 0

            if self.accel_line > len(self.accel_lines) - 1:
                self.accel_line = 0

            split_gps = self.gps_lines[self.gps_line].split(',')
            split_accel = self.accel_lines[self.accel_line].split(',')

            long, lat = split_gps
            data.gps = Gps(long, lat)

            x, y, z = split_accel
            data.accelerometer = Accelerometer(x, y, z)

            self.gps_line += 1
            self.accel_line += 1

        return data

    def startReading(self, *args, **kwargs):
        """Метод повинен викликатись перед початком читання даних"""
        self.reading = True

    def stopReading(self, *args, **kwargs):
        """Метод повинен викликатись для закінчення читання даних"""
        self.reading = False
        self.accel_line = 0
        self.gps_line = 0
