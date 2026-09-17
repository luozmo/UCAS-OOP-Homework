package libraryexample;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class Librarian {
    private final String employeeId;
    private final String name;
    private final String department;
    private final Set<String> registeredStudentIds;
    private final List<Book> managedBooks;

    public Librarian(String employeeId, String name, String department) {
        this.employeeId = employeeId;
        this.name = name;
        this.department = department;
        this.registeredStudentIds = new HashSet<String>();
        this.managedBooks = new ArrayList<Book>();
    }

    public String getEmployeeId() {
        return employeeId;
    }

    public String getName() {
        return name;
    }

    public String getDepartment() {
        return department;
    }

    public boolean registerStudent(Student student) {
        return student != null && registeredStudentIds.add(student.getStudentId());
    }

    public boolean processBorrow(Student student, Book book) {
        if (student == null || book == null) {
            return false;
        }
        if (!registeredStudentIds.contains(student.getStudentId())) {
            return false;
        }
        if (!managedBooks.contains(book)) {
            return false;
        }
        return student.borrowBook(book);
    }

    public boolean processReturn(Student student, Book book) {
        if (student == null || book == null) {
            return false;
        }
        if (!registeredStudentIds.contains(student.getStudentId())) {
            return false;
        }
        return student.returnBook(book);
    }

    public boolean addBook(Book book) {
        return book != null && !managedBooks.contains(book) && managedBooks.add(book);
    }

    public boolean removeBook(Book book) {
        return book != null && managedBooks.remove(book);
    }

    public List<Book> getManagedBooks() {
        return new ArrayList<Book>(managedBooks);
    }

    @Override
    public String toString() {
        return name + " (" + employeeId + "), " + department;
    }
}
