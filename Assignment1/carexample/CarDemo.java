package carexample;

import java.time.LocalDate;

public class CarDemo {
    public static void main(String[] args) {
        ManufacturerCar manufacturerCar =
                new ManufacturerCar("MC-2026", "E-01", 180000.0);
        manufacturerCar.designModel("Electric sedan, 500 km range");
        manufacturerCar.startProduction();
        boolean inspected = manufacturerCar.inspectQuality("VIN-20260001");

        System.out.println("Manufacturer model: " + manufacturerCar.getModelName());
        System.out.println("Production status: " + manufacturerCar.getProductionStatus());
        System.out.println("Quality inspection: " + inspected);
        System.out.println("Estimated cost: " + manufacturerCar.calculateCost());

        OwnedCar ownedCar = new OwnedCar(
                "A12345",
                "E-01",
                "Li Ming",
                12000.0,
                20.0,
                LocalDate.now().plusMonths(6));

        ownedCar.refuel(10.0);
        boolean started = ownedCar.start();
        boolean driven = ownedCar.drive(50.0);
        boolean parked = ownedCar.park();
        ownedCar.scheduleMaintenance();

        System.out.println();
        System.out.println("Owned car: " + ownedCar);
        System.out.println("Started: " + started + ", driven: " + driven + ", parked: " + parked);
        System.out.println("Maintenance scheduled: " + ownedCar.isMaintenanceScheduled());

        RegisteredCar registeredCar = new RegisteredCar("VIN-20260001", "A12345");
        boolean registered = registeredCar.register("ID-10001");
        boolean transferred = registeredCar.transferOwnership("ID-10002");
        registeredCar.recordViolation("Speeding");
        registeredCar.annualInspection(true);

        System.out.println();
        System.out.println("Registration success: " + registered);
        System.out.println("Current owner: " + registeredCar.getOwnerId());
        System.out.println("Ownership transfer: " + transferred);
        System.out.println("Vehicle status: " + registeredCar.getStatus());
        System.out.println("Violations: " + registeredCar.getViolationRecords());
        System.out.println("Next inspection: " + registeredCar.getInspectionExpiry());
    }
}
