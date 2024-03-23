from csv import reader
from datetime import datetime
from domain.accelerometer import Accelerometer
from domain.gps import Gps
from domain.parking import Parking
from domain.aggregated_data import AggregatedData
import config

class FileDatasource:
    def __init__(
        self,
        accelerometer_filename: str,
        gps_filename: str,
        parking_filename: str,
    ) -> None:
        self.accel_filename = accelerometer_filename
        self.gps_filename = gps_filename
        self.parking_filename = parking_filename

        with open(self.gps_filename) as file:
            lines = [line.rstrip() for line in file]
            lines = lines[1:]
            self.gps_lines = lines

        with open(self.accel_filename) as file:
            lines = [line.rstrip() for line in file]
            lines = lines[1:]
            self.accel_lines = lines

        with open(self.parking_filename) as file:
            lines = [line.rstrip() for line in file]
            lines = lines[1:]
            self.parking_lines = lines

    def read(self) -> AggregatedData:
        """Метод повертає дані отримані з датчиків"""
        data = AggregatedData(
            Accelerometer(1, 2, 3),
            Gps(4, 5),
            Parking(25, Gps(4, 5)),
            datetime.now(),
            config.USER_ID,
        )

        if self.reading == True:
            if self.gps_line > len(self.gps_lines) - 1:
                self.gps_line = 0

            if self.accel_line > len(self.accel_lines) - 1:
                self.accel_line = 0

            if self.parking_line > len(self.parking_lines) - 1:
                self.parking_line = 0

            split_gps = self.gps_lines[self.gps_line].split(',')
            split_accel = self.accel_lines[self.accel_line].split(',')
            split_parking = self.parking_lines[self.parking_line].split(',')

            lat, long = split_gps
            data.gps = Gps(long, lat)

            x, y, z = split_accel
            data.accelerometer = Accelerometer(x, y, z)

            long, lat, empty_count = split_parking
            data.parking = Parking(empty_count, Gps(long, lat))

            self.gps_line += 1
            self.accel_line += 1
            self.parking_line += 1

        return data

    def startReading(self, *args, **kwargs):
        """Метод повинен викликатись перед початком читання даних"""
        self.reading = True
        self.accel_line = 0
        self.gps_line = 0
        self.parking_line = 0

    def stopReading(self, *args, **kwargs):
        """Метод повинен викликатись для закінчення читання даних"""
        self.reading = False
