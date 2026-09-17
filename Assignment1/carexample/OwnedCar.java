package carexample;

import java.time.LocalDate;

public class OwnedCar {
    private static final double MAX_FUEL_LEVEL = 60.0;
    private static final double FUEL_PER_KILOMETER = 0.08;

    private final String licensePlate;
    private final String brandModel;
    private final String ownerName;
    private double mileage;
    private double fuelLevel;
    private LocalDate insuranceExpiry;
    private boolean running;
    private boolean maintenanceScheduled;

    public OwnedCar(String licensePlate, String brandModel, String ownerName,
                    double mileage, double fuelLevel, LocalDate insuranceExpiry) {
        if (mileage < 0 || fuelLevel < 0 || fuelLevel > MAX_FUEL_LEVEL) {
            throw new IllegalArgumentException("Invalid mileage or fuel level");
        }
        this.licensePlate = licensePlate;
        this.brandModel = brandModel;
        this.ownerName = ownerName;
        this.mileage = mileage;
        this.fuelLevel = fuelLevel;
        this.insuranceExpiry = insuranceExpiry;
    }

    public String getLicensePlate() { return licensePlate; }
    public String getBrandModel() { return brandModel; }
    public String getOwnerName() { return ownerName; }
    public double getMileage() { return mileage; }
    public double getFuelLevel() { return fuelLevel; }
    public LocalDate getInsuranceExpiry() { return insuranceExpiry; }
    public boolean isRunning() { return running; }
    public boolean isMaintenanceScheduled() { return maintenanceScheduled; }

    public boolean start() {
        if (running || fuelLevel <= 0) {
            return false;
        }
        running = true;
        return true;
    }

    public boolean drive(double distance) {
        if (!running || distance <= 0) {
            return false;
        }
        double fuelNeeded = distance * FUEL_PER_KILOMETER;
        if (fuelNeeded > fuelLevel) {
            return false;
        }
        mileage += distance;
        fuelLevel -= fuelNeeded;
        return true;
    }

    public boolean refuel(double amount) {
        if (amount <= 0 || fuelLevel >= MAX_FUEL_LEVEL) {
            return false;
        }
        fuelLevel = Math.min(MAX_FUEL_LEVEL, fuelLevel + amount);
        return true;
    }

    public boolean park() {
        if (!running) {
            return false;
        }
        running = false;
        return true;
    }

    public void scheduleMaintenance() {
        maintenanceScheduled = true;
    }

    public void renewInsurance(LocalDate expiry) {
        if (expiry != null
                && (insuranceExpiry == null || expiry.isAfter(insuranceExpiry))) {
            insuranceExpiry = expiry;
        }
    }

    @Override
    public String toString() {
        return brandModel + " (" + licensePlate + "), mileage: " + mileage
                + ", fuel: " + fuelLevel;
    }
}
