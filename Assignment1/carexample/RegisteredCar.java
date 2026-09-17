package carexample;

import java.time.LocalDate;
import java.util.ArrayList;
import java.util.List;

public class RegisteredCar {
    public enum VehicleStatus {
        UNREGISTERED,
        REGISTERED,
        REVOKED
    }

    private final String vin;
    private final String plateNumber;
    private String ownerId;
    private LocalDate registrationDate;
    private LocalDate inspectionExpiry;
    private VehicleStatus status;
    private final List<String> violationRecords;

    public RegisteredCar(String vin, String plateNumber) {
        this.vin = vin;
        this.plateNumber = plateNumber;
        this.status = VehicleStatus.UNREGISTERED;
        this.violationRecords = new ArrayList<String>();
    }

    public String getVin() { return vin; }
    public String getPlateNumber() { return plateNumber; }
    public String getOwnerId() { return ownerId; }
    public LocalDate getRegistrationDate() { return registrationDate; }
    public LocalDate getInspectionExpiry() { return inspectionExpiry; }
    public VehicleStatus getStatus() { return status; }

    public List<String> getViolationRecords() {
        return new ArrayList<String>(violationRecords);
    }

    public boolean register(String ownerId) {
        if (status == VehicleStatus.REGISTERED
                || ownerId == null
                || ownerId.trim().isEmpty()) {
            return false;
        }
        this.ownerId = ownerId;
        this.registrationDate = LocalDate.now();
        this.inspectionExpiry = registrationDate.plusYears(2);
        this.status = VehicleStatus.REGISTERED;
        return true;
    }

    public boolean transferOwnership(String newOwnerId) {
        if (status != VehicleStatus.REGISTERED
                || newOwnerId == null
                || newOwnerId.trim().isEmpty()) {
            return false;
        }
        ownerId = newOwnerId;
        return true;
    }

    public void recordViolation(String violation) {
        if (status == VehicleStatus.REGISTERED
                && violation != null
                && !violation.trim().isEmpty()) {
            violationRecords.add(violation);
        }
    }

    public void annualInspection(boolean result) {
        if (status == VehicleStatus.REGISTERED && result) {
            inspectionExpiry = LocalDate.now().plusYears(1);
        }
    }

    public void revokeRegistration() {
        status = VehicleStatus.REVOKED;
    }
}
